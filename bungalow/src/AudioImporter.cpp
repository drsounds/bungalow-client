#include "AudioImporter.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <regex>

namespace fs = std::filesystem;

namespace space {

// ── Byte / string helpers ─────────────────────────────────────────────────────

using Bytes = std::vector<uint8_t>;

static uint32_t readBE16(const uint8_t* p) { return (uint32_t(p[0]) << 8) | p[1]; }
static uint32_t readBE24(const uint8_t* p) { return (uint32_t(p[0]) << 16) | (uint32_t(p[1]) << 8) | p[2]; }
static uint32_t readBE32(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
}
static uint64_t readBE64(const uint8_t* p) { return (uint64_t(readBE32(p)) << 32) | readBE32(p + 4); }
static uint32_t readLE32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
static uint64_t readLE64(const uint8_t* p) { return uint64_t(readLE32(p)) | (uint64_t(readLE32(p + 4)) << 32); }
static uint32_t readSyncsafe4(const uint8_t* p) {
    return (uint32_t(p[0] & 0x7F) << 21) | (uint32_t(p[1] & 0x7F) << 14)
         | (uint32_t(p[2] & 0x7F) <<  7) |  uint32_t(p[3] & 0x7F);
}

static Bytes readBytes(std::ifstream& f, uint64_t offset, size_t len) {
    Bytes out(len);
    f.clear();
    f.seekg(static_cast<std::streamoff>(offset));
    f.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(len));
    out.resize(static_cast<size_t>(std::max<std::streamsize>(f.gcount(), 0)));
    return out;
}

static void trim(std::string& s) {
    s.erase(std::remove(s.begin(), s.end(), '\0'), s.end());
    size_t b = s.find_first_not_of(" \t\r\n");
    size_t e = s.find_last_not_of(" \t\r\n");
    if (b == std::string::npos) s.clear();
    else s = s.substr(b, e - b + 1);
}

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

static void setIfEmpty(std::string& dst, std::string v) {
    trim(v);
    if (dst.empty() && !v.empty()) dst = std::move(v);
}

static void appendUtf8(std::string& out, uint32_t cp) {
    if (cp < 0x80) {
        out += char(cp);
    } else if (cp < 0x800) {
        out += char(0xC0 | (cp >> 6));
        out += char(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += char(0xE0 | (cp >> 12));
        out += char(0x80 | ((cp >> 6) & 0x3F));
        out += char(0x80 | (cp & 0x3F));
    } else {
        out += char(0xF0 | (cp >> 18));
        out += char(0x80 | ((cp >> 12) & 0x3F));
        out += char(0x80 | ((cp >> 6) & 0x3F));
        out += char(0x80 | (cp & 0x3F));
    }
}

// ── ID3v2 ─────────────────────────────────────────────────────────────────────

// Decode an ID3v2 text frame (first byte = encoding) to UTF-8.
// Multiple NUL-separated values (ID3v2.4) are joined with "; ".
static std::string decodeId3Text(const uint8_t* data, size_t size) {
    if (size == 0) return {};
    const uint8_t enc  = data[0];
    const uint8_t* p   = data + 1;
    const size_t   len = size - 1;
    std::vector<std::string> values(1);

    if (enc == 0 || enc == 3) {
        for (size_t i = 0; i < len; ++i) {
            if (p[i] == 0) { values.emplace_back(); continue; }
            if (enc == 3) values.back() += char(p[i]);
            else          appendUtf8(values.back(), p[i]);  // Latin-1
        }
    } else {
        bool bigEndian = (enc == 2);
        size_t i = 0;
        while (i + 1 < len) {
            if (p[i] == 0xFF && p[i + 1] == 0xFE) { bigEndian = false; i += 2; continue; }
            if (p[i] == 0xFE && p[i + 1] == 0xFF) { bigEndian = true;  i += 2; continue; }
            uint32_t u = bigEndian ? readBE16(p + i) : (uint32_t(p[i + 1]) << 8 | p[i]);
            i += 2;
            if (u == 0) { values.emplace_back(); continue; }
            if (u >= 0xD800 && u < 0xDC00 && i + 1 < len) {
                uint32_t lo = bigEndian ? readBE16(p + i) : (uint32_t(p[i + 1]) << 8 | p[i]);
                if (lo >= 0xDC00 && lo < 0xE000) {
                    i += 2;
                    u = 0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00);
                }
            }
            appendUtf8(values.back(), u);
        }
    }

    std::string out;
    for (auto& v : values) {
        trim(v);
        if (v.empty()) continue;
        if (!out.empty()) out += "; ";
        out += v;
    }
    return out;
}

// Parse an in-memory ID3v2 tag (buf starts with "ID3"). Returns the tag's total size.
static size_t parseId3v2(const Bytes& buf, AudioFileMetadata& m, std::string& subtitle) {
    if (buf.size() < 10 || buf[0] != 'I' || buf[1] != 'D' || buf[2] != '3') return 0;
    const int ver = buf[3];
    if (ver < 2 || ver > 4) return 0;
    const size_t tagSize = 10 + readSyncsafe4(&buf[6]) + ((buf[5] & 0x10) ? 10 : 0);
    const size_t end = std::min(tagSize, buf.size());

    size_t pos = 10;
    if ((buf[5] & 0x40) && ver >= 3 && pos + 4 <= end)
        pos += (ver == 3) ? 4 + readBE32(&buf[pos]) : readSyncsafe4(&buf[pos]);

    const size_t hdrLen = (ver == 2) ? 6 : 10;
    while (pos + hdrLen <= end) {
        const uint8_t* fh = &buf[pos];
        if (fh[0] == 0) break;  // padding
        std::string id(reinterpret_cast<const char*>(fh), ver == 2 ? 3 : 4);
        size_t fsize = (ver == 2) ? readBE24(fh + 3)
                     : (ver == 4) ? readSyncsafe4(fh + 4) : readBE32(fh + 4);
        pos += hdrLen;
        if (fsize > end - pos) break;

        const uint8_t* data = &buf[pos];
        size_t dlen = fsize;
        pos += fsize;

        if (ver == 3 && (fh[9] & 0xC0)) continue;          // compressed / encrypted
        if (ver == 4) {
            if (fh[9] & 0x0C) continue;                    // compressed / encrypted
            if ((fh[9] & 0x01) && dlen >= 4) { data += 4; dlen -= 4; }  // data length indicator
        }
        if (id.empty() || id[0] != 'T') continue;

        std::string v = decodeId3Text(data, dlen);
        if      (id == "TIT2" || id == "TT2") setIfEmpty(m.title, v);
        else if (id == "TIT3" || id == "TT3") setIfEmpty(subtitle, v);
        else if (id == "TPE1" || id == "TP1") setIfEmpty(m.artists, v);
        else if (id == "TPE2" || id == "TP2") setIfEmpty(m.albumArtist, v);
        else if (id == "TALB" || id == "TAL") setIfEmpty(m.album, v);
        else if (id == "TSRC" || id == "TRC") setIfEmpty(m.isrc, v);
        else if (id == "TDRC" || id == "TYER" || id == "TYE") setIfEmpty(m.releaseDate, v);
        else if ((id == "TRCK" || id == "TRK") && m.trackNumber == 0) m.trackNumber = std::atoi(v.c_str());
        else if ((id == "TLEN" || id == "TLE") && m.duration == 0)    m.duration    = std::atoi(v.c_str());
    }
    return tagSize;
}

// ── ID3v1 ─────────────────────────────────────────────────────────────────────

static bool parseId3v1(std::ifstream& f, uint64_t fileSize, AudioFileMetadata& m) {
    if (fileSize < 128) return false;
    Bytes tag = readBytes(f, fileSize - 128, 128);
    if (tag.size() != 128 || tag[0] != 'T' || tag[1] != 'A' || tag[2] != 'G') return false;

    auto field = [&](size_t off, size_t len) {
        std::string out;
        for (size_t i = off; i < off + len && tag[i] != 0; ++i) appendUtf8(out, tag[i]);
        return out;
    };
    setIfEmpty(m.title,   field(3, 30));
    setIfEmpty(m.artists, field(33, 30));
    setIfEmpty(m.album,   field(63, 30));
    setIfEmpty(m.releaseDate, field(93, 4));
    if (m.trackNumber == 0 && tag[125] == 0 && tag[126] != 0) m.trackNumber = tag[126];
    return true;
}

// ── MP3 duration (Xing/Info/VBRI header, else CBR estimate) ───────────────────

static int mp3Duration(std::ifstream& f, uint64_t audioStart, uint64_t audioEnd) {
    static const int kBitrateV1[]  = { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 };
    static const int kBitrateV2[]  = { 0,  8, 16, 24, 32, 40, 48, 56,  64,  80,  96, 112, 128, 144, 160 };
    static const int kRates[3][3]  = { { 44100, 48000, 32000 }, { 22050, 24000, 16000 }, { 11025, 12000, 8000 } };

    if (audioEnd <= audioStart) return 0;
    Bytes buf = readBytes(f, audioStart, 64 * 1024);
    for (size_t i = 0; i + 4 <= buf.size(); ++i) {
        if (buf[i] != 0xFF || (buf[i + 1] & 0xE0) != 0xE0) continue;
        const int verBits = (buf[i + 1] >> 3) & 3;  // 0 = 2.5, 2 = 2, 3 = 1
        const int layer   = (buf[i + 1] >> 1) & 3;  // 1 = Layer III
        const int brIdx   = (buf[i + 2] >> 4) & 0xF;
        const int srIdx   = (buf[i + 2] >> 2) & 3;
        const bool mono   = ((buf[i + 3] >> 6) & 3) == 3;
        if (verBits == 1 || layer != 1 || brIdx == 0 || brIdx == 15 || srIdx == 3) continue;

        const bool mpeg1  = verBits == 3;
        const int  rate   = kRates[mpeg1 ? 0 : (verBits == 2 ? 1 : 2)][srIdx];
        const int  kbps   = mpeg1 ? kBitrateV1[brIdx] : kBitrateV2[brIdx];
        const int  spf    = mpeg1 ? 1152 : 576;
        const size_t side = mpeg1 ? (mono ? 17 : 32) : (mono ? 9 : 17);

        uint32_t frames = 0;
        const size_t x = i + 4 + side;
        if (x + 12 <= buf.size() &&
            (std::memcmp(&buf[x], "Xing", 4) == 0 || std::memcmp(&buf[x], "Info", 4) == 0) &&
            (readBE32(&buf[x + 4]) & 1)) {
            frames = readBE32(&buf[x + 8]);
        } else if (i + 36 + 18 <= buf.size() && std::memcmp(&buf[i + 36], "VBRI", 4) == 0) {
            frames = readBE32(&buf[i + 36 + 14]);
        }

        if (frames > 0) return static_cast<int>(uint64_t(frames) * spf * 1000 / rate);
        const uint64_t audioBytes = audioEnd - (audioStart + i);
        return static_cast<int>(audioBytes * 8 / kbps);
    }
    return 0;
}

// ── Vorbis comments (FLAC / Ogg) ──────────────────────────────────────────────

static void parseVorbisComments(const uint8_t* p, size_t len, AudioFileMetadata& m,
                                std::string& subtitle) {
    if (len < 8) return;
    size_t pos = 4 + size_t(readLE32(p));
    if (pos + 4 > len) return;
    uint32_t count = readLE32(p + pos);
    pos += 4;

    std::vector<std::string> artists;
    for (uint32_t i = 0; i < count && pos + 4 <= len; ++i) {
        size_t n = readLE32(p + pos);
        pos += 4;
        if (n > len - pos) break;
        std::string entry(reinterpret_cast<const char*>(p + pos), n);
        pos += n;

        size_t eq = entry.find('=');
        if (eq == std::string::npos) continue;
        std::string key = entry.substr(0, eq);
        std::transform(key.begin(), key.end(), key.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        std::string v = entry.substr(eq + 1);

        if      (key == "TITLE")       setIfEmpty(m.title, v);
        else if (key == "VERSION")     setIfEmpty(subtitle, v);
        else if (key == "ARTIST")      { trim(v); if (!v.empty()) artists.push_back(v); }
        else if (key == "ALBUMARTIST" || key == "ALBUM ARTIST") setIfEmpty(m.albumArtist, v);
        else if (key == "ALBUM")       setIfEmpty(m.album, v);
        else if (key == "ISRC")        setIfEmpty(m.isrc, v);
        else if (key == "DATE")        setIfEmpty(m.releaseDate, v);
        else if (key == "TRACKNUMBER" && m.trackNumber == 0) m.trackNumber = std::atoi(v.c_str());
    }
    if (m.artists.empty()) {
        for (auto& a : artists) {
            if (!m.artists.empty()) m.artists += "; ";
            m.artists += a;
        }
    }
}

static void readFlac(std::ifstream& f, uint64_t start, AudioFileMetadata& m, std::string& subtitle) {
    Bytes magic = readBytes(f, start, 4);
    if (magic.size() != 4 || std::memcmp(magic.data(), "fLaC", 4) != 0) return;

    uint64_t pos = start + 4;
    for (int guard = 0; guard < 128; ++guard) {
        Bytes hdr = readBytes(f, pos, 4);
        if (hdr.size() != 4) return;
        const bool     last = (hdr[0] & 0x80) != 0;
        const int      type = hdr[0] & 0x7F;
        const uint32_t len  = readBE24(&hdr[1]);
        pos += 4;

        if (type == 0 && len >= 18) {  // STREAMINFO
            Bytes d = readBytes(f, pos, 18);
            if (d.size() == 18) {
                uint32_t rate  = (uint32_t(d[10]) << 12) | (uint32_t(d[11]) << 4) | (d[12] >> 4);
                uint64_t total = (uint64_t(d[13] & 0x0F) << 32) | readBE32(&d[14]);
                if (rate > 0 && m.duration == 0) m.duration = static_cast<int>(total * 1000 / rate);
            }
        } else if (type == 4) {        // VORBIS_COMMENT
            Bytes d = readBytes(f, pos, len);
            parseVorbisComments(d.data(), d.size(), m, subtitle);
        }
        pos += len;
        if (last) return;
    }
}

// Ogg: concatenate page bodies (which yields the packet stream) and look for
// the comment header; take the duration from the last page's granule position.
static void readOgg(std::ifstream& f, uint64_t fileSize, AudioFileMetadata& m, std::string& subtitle) {
    Bytes stream;
    uint64_t pos = 0;
    while (stream.size() < 512 * 1024) {
        Bytes h = readBytes(f, pos, 27);
        if (h.size() != 27 || std::memcmp(h.data(), "OggS", 4) != 0) break;
        Bytes segs = readBytes(f, pos + 27, h[26]);
        if (segs.size() != h[26]) break;
        size_t bodyLen = 0;
        for (uint8_t s : segs) bodyLen += s;
        Bytes body = readBytes(f, pos + 27 + h[26], bodyLen);
        stream.insert(stream.end(), body.begin(), body.end());
        pos += 27 + h[26] + bodyLen;
    }

    uint32_t rate = 0, preSkip = 0;
    auto find = [&](const char* sig, size_t n) -> size_t {
        auto it = std::search(stream.begin(), stream.end(), sig, sig + n);
        return it == stream.end() ? std::string::npos : size_t(it - stream.begin());
    };
    size_t ident = find("\x01vorbis", 7);
    if (ident != std::string::npos && ident + 16 <= stream.size()) rate = readLE32(&stream[ident + 12]);
    size_t opus = find("OpusHead", 8);
    if (opus != std::string::npos && opus + 12 <= stream.size()) {
        rate    = 48000;
        preSkip = uint32_t(stream[opus + 10]) | (uint32_t(stream[opus + 11]) << 8);
    }

    size_t c = find("\x03vorbis", 7);
    if (c != std::string::npos) parseVorbisComments(&stream[c + 7], stream.size() - c - 7, m, subtitle);
    else if ((c = find("OpusTags", 8)) != std::string::npos)
        parseVorbisComments(&stream[c + 8], stream.size() - c - 8, m, subtitle);

    if (rate == 0 || m.duration != 0) return;
    const uint64_t tailLen = std::min<uint64_t>(fileSize, 64 * 1024);
    Bytes tail = readBytes(f, fileSize - tailLen, static_cast<size_t>(tailLen));
    for (size_t i = tail.size() >= 14 ? tail.size() - 14 : 0; i-- > 0;) {
        if (std::memcmp(&tail[i], "OggS", 4) != 0) continue;
        uint64_t granule = readLE64(&tail[i + 6]);
        if (granule > preSkip) m.duration = static_cast<int>((granule - preSkip) * 1000 / rate);
        break;
    }
}

// ── MP4 / M4A atoms ───────────────────────────────────────────────────────────

struct Box {
    std::string type;
    size_t      body = 0;  // offset of the box payload
    size_t      end  = 0;  // offset just past the box
};

static bool nextBox(const Bytes& b, size_t pos, size_t limit, Box& out) {
    if (pos + 8 > limit) return false;
    uint64_t size = readBE32(&b[pos]);
    out.type.assign(reinterpret_cast<const char*>(&b[pos + 4]), 4);
    out.body = pos + 8;
    if (size == 1) {
        if (pos + 16 > limit) return false;
        size = readBE64(&b[pos + 8]);
        out.body = pos + 16;
    } else if (size == 0) {
        size = limit - pos;
    }
    if (size < out.body - pos || size > limit - pos) return false;
    out.end = pos + static_cast<size_t>(size);
    return true;
}

static bool findChild(const Bytes& b, size_t start, size_t end, const char* type, Box& out) {
    for (size_t pos = start; nextBox(b, pos, end, out); pos = out.end)
        if (out.type == type) return true;
    return false;
}

// Payload of the 'data' atom inside an ilst item.
static std::string mp4DataValue(const Bytes& b, const Box& item, uint32_t* typeOut = nullptr) {
    Box data;
    if (!findChild(b, item.body, item.end, "data", data) || data.end < data.body + 8) return {};
    if (typeOut) *typeOut = readBE32(&b[data.body]) & 0xFFFFFF;
    return std::string(reinterpret_cast<const char*>(&b[data.body + 8]), data.end - data.body - 8);
}

static void parseMp4Moov(const Bytes& b, AudioFileMetadata& m) {
    Box mvhd;
    if (findChild(b, 0, b.size(), "mvhd", mvhd) && mvhd.end >= mvhd.body + 32) {
        const uint8_t* p = &b[mvhd.body];
        uint32_t scale = 0;
        uint64_t dur   = 0;
        if (p[0] == 1)                                { scale = readBE32(p + 20); dur = readBE64(p + 24); }
        else                                          { scale = readBE32(p + 12); dur = readBE32(p + 16); }
        if (scale > 0) m.duration = static_cast<int>(dur * 1000 / scale);
    }

    Box udta, meta, ilst;
    if (!findChild(b, 0, b.size(), "udta", udta)) return;
    if (!findChild(b, udta.body, udta.end, "meta", meta)) return;
    // 'meta' is usually a full box (4 bytes version/flags) but not in QuickTime files.
    size_t metaBody = meta.body;
    if (meta.end >= meta.body + 12 && std::memcmp(&b[meta.body + 4], "hdlr", 4) != 0) metaBody += 4;
    if (!findChild(b, metaBody, meta.end, "ilst", ilst)) return;

    Box item;
    for (size_t pos = ilst.body; nextBox(b, pos, ilst.end, item); pos = item.end) {
        const std::string& t = item.type;
        if      (t == "\xA9nam") setIfEmpty(m.title,       mp4DataValue(b, item));
        else if (t == "\xA9" "ART") setIfEmpty(m.artists,  mp4DataValue(b, item));
        else if (t == "aART")    setIfEmpty(m.albumArtist, mp4DataValue(b, item));
        else if (t == "\xA9" "alb") setIfEmpty(m.album,    mp4DataValue(b, item));
        else if (t == "\xA9" "day") setIfEmpty(m.releaseDate, mp4DataValue(b, item));
        else if (t == "trkn") {
            std::string v = mp4DataValue(b, item);
            if (v.size() >= 4) m.trackNumber = static_cast<int>(readBE16(reinterpret_cast<const uint8_t*>(v.data()) + 2));
        } else if (t == "----") {  // freeform: mean / name / data
            Box name;
            if (!findChild(b, item.body, item.end, "name", name) || name.end < name.body + 4) continue;
            std::string key(reinterpret_cast<const char*>(&b[name.body + 4]), name.end - name.body - 4);
            if (toLower(key) == "isrc") setIfEmpty(m.isrc, mp4DataValue(b, item));
        }
    }
}

static void readMp4(std::ifstream& f, uint64_t fileSize, AudioFileMetadata& m) {
    uint64_t pos = 0;
    while (pos + 8 <= fileSize) {
        Bytes h = readBytes(f, pos, 16);
        if (h.size() < 8) return;
        uint64_t size = readBE32(h.data());
        uint64_t hdr  = 8;
        if (size == 1 && h.size() == 16) { size = readBE64(&h[8]); hdr = 16; }
        else if (size == 0)              { size = fileSize - pos; }
        if (size < hdr) return;

        if (std::memcmp(&h[4], "moov", 4) == 0) {
            if (size - hdr > 64ull * 1024 * 1024) return;
            parseMp4Moov(readBytes(f, pos + hdr, static_cast<size_t>(size - hdr)), m);
            return;
        }
        pos += size;
    }
}

// ── WAV (RIFF INFO + embedded ID3 chunk) ──────────────────────────────────────

static void readWav(std::ifstream& f, uint64_t fileSize, AudioFileMetadata& m, std::string& subtitle) {
    uint32_t byteRate = 0;
    uint64_t dataSize = 0;
    uint64_t pos = 12;
    while (pos + 8 <= fileSize) {
        Bytes h = readBytes(f, pos, 8);
        if (h.size() != 8) break;
        std::string id(reinterpret_cast<const char*>(h.data()), 4);
        uint32_t len = readLE32(&h[4]);
        uint64_t body = pos + 8;

        if (id == "fmt " && len >= 12) {
            Bytes d = readBytes(f, body, 12);
            if (d.size() == 12) byteRate = readLE32(&d[8]);
        } else if (id == "data") {
            dataSize = std::min<uint64_t>(len, fileSize - body);
        } else if ((id == "id3 " || id == "ID3 ") && len < 16 * 1024 * 1024) {
            parseId3v2(readBytes(f, body, len), m, subtitle);
        } else if (id == "LIST" && len >= 4 && len < 1024 * 1024) {
            Bytes d = readBytes(f, body, len);
            if (d.size() >= 4 && std::memcmp(d.data(), "INFO", 4) == 0) {
                for (size_t p = 4; p + 8 <= d.size();) {
                    std::string sub(reinterpret_cast<const char*>(&d[p]), 4);
                    size_t n = readLE32(&d[p + 4]);
                    if (n > d.size() - p - 8) break;
                    std::string v;
                    for (size_t i = 0; i < n && d[p + 8 + i]; ++i) appendUtf8(v, d[p + 8 + i]);
                    if      (sub == "INAM") setIfEmpty(m.title, v);
                    else if (sub == "IART") setIfEmpty(m.artists, v);
                    else if (sub == "IPRD") setIfEmpty(m.album, v);
                    else if (sub == "ICRD") setIfEmpty(m.releaseDate, v);
                    else if (sub == "ITRK" && m.trackNumber == 0) m.trackNumber = std::atoi(v.c_str());
                    p += 8 + n + (n & 1);
                }
            }
        }
        pos = body + len + (len & 1);
    }
    if (m.duration == 0 && byteRate > 0) m.duration = static_cast<int>(dataSize * 1000 / byteRate);
}

// ── Normalisation ─────────────────────────────────────────────────────────────

static bool startsWithFeaturing(const std::string& lower, size_t& prefixLen) {
    for (const char* p : { "featuring ", "feat. ", "feat ", "ft. ", "ft " }) {
        size_t n = std::strlen(p);
        if (lower.compare(0, n, p) == 0) { prefixLen = n; return true; }
    }
    return false;
}

// Split an artist string on ";" and featuring markers. Commas and "&" are kept,
// since they are commonly part of a single artist name.
static std::vector<std::string> splitArtists(const std::string& s) {
    std::string work = s;
    for (const char* sep : { " featuring ", " feat. ", " feat ", " ft. ", " ft " }) {
        const size_t n = std::strlen(sep);
        for (size_t p; (p = toLower(work).find(sep)) != std::string::npos;)
            work.replace(p, n, ";");
    }
    std::vector<std::string> out;
    size_t start = 0;
    while (start <= work.size()) {
        size_t end = work.find(';', start);
        if (end == std::string::npos) end = work.size();
        std::string a = work.substr(start, end - start);
        trim(a);
        bool dup = std::any_of(out.begin(), out.end(),
                               [&](const std::string& o) { return toLower(o) == toLower(a); });
        if (!a.empty() && !dup) out.push_back(a);
        start = end + 1;
    }
    return out;
}

std::string AudioImporter::extractVersion(std::string& title) {
    static const std::regex kPatterns[] = {
        std::regex(R"(\s*\[([^\]]+)\]\s*$)"),
        std::regex(R"(\s*\(([^)]+)\)\s*$)"),
        std::regex("\\s+(?:-|\xE2\x80\x93)\\s+([^()\\[\\]]+?)\\s*$"),  // " - " or " – "
    };
    for (const auto& re : kPatterns) {
        std::smatch match;
        if (std::regex_search(title, match, re)) {
            std::string version = match[1].str();
            trim(version);
            title = title.substr(0, static_cast<size_t>(match.position()));
            trim(title);
            return version;
        }
    }
    return {};
}

AudioFileMetadata AudioImporter::readMetadata(const std::string& path) {
    AudioFileMetadata m;
    std::string subtitle;

    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (f) {
        const uint64_t fileSize = static_cast<uint64_t>(f.tellg());
        Bytes head = readBytes(f, 0, 12);

        uint64_t afterId3 = 0;
        if (head.size() >= 10 && std::memcmp(head.data(), "ID3", 3) == 0) {
            const size_t tagSize = 10 + readSyncsafe4(&head[6]) + ((head[5] & 0x10) ? 10 : 0);
            afterId3 = parseId3v2(readBytes(f, 0, std::min<size_t>(tagSize, 32 * 1024 * 1024)), m, subtitle);
            head = readBytes(f, afterId3, 12);
        }

        if (head.size() >= 4 && std::memcmp(head.data(), "fLaC", 4) == 0) {
            readFlac(f, afterId3, m, subtitle);
        } else if (head.size() >= 4 && std::memcmp(head.data(), "OggS", 4) == 0) {
            readOgg(f, fileSize, m, subtitle);
        } else if (head.size() >= 8 && std::memcmp(&head[4], "ftyp", 4) == 0) {
            readMp4(f, fileSize, m);
        } else if (head.size() >= 12 && std::memcmp(head.data(), "RIFF", 4) == 0 &&
                   std::memcmp(&head[8], "WAVE", 4) == 0) {
            readWav(f, fileSize, m, subtitle);
        } else {
            // MPEG audio (MP3 / ADTS AAC): ID3v1 at the end, duration from frames.
            const bool hasV1 = parseId3v1(f, fileSize, m);
            if (m.duration == 0)
                m.duration = mp3Duration(f, afterId3, hasV1 ? fileSize - 128 : fileSize);
        }
    }

    trim(m.title);
    trim(m.artists);
    trim(m.album);
    trim(m.albumArtist);
    trim(m.isrc);
    trim(m.releaseDate);
    m.hasTags = !m.title.empty() || !m.artists.empty();

    // Filename fallback: "Artist - Title (Version).mp3"
    {
        std::string stem = fs::path(path).stem().string();
        size_t sep = stem.find(" - ");
        if (sep != std::string::npos) {
            if (m.artists.empty()) { m.artists = stem.substr(0, sep); trim(m.artists); }
            if (m.title.empty())   { m.title   = stem.substr(sep + 3); trim(m.title); }
        } else if (m.title.empty()) {
            m.title = stem;
            trim(m.title);
        }
    }

    m.artistList = splitArtists(m.artists);

    // Pull "(feat. X)" and a version suffix out of the title.
    m.version = subtitle;
    for (int i = 0; i < 3; ++i) {
        std::string rest = m.title;
        std::string v = extractVersion(rest);
        if (v.empty() || rest.empty()) break;
        size_t prefix = 0;
        if (startsWithFeaturing(toLower(v), prefix)) {
            for (auto& a : splitArtists(v.substr(prefix))) {
                bool dup = std::any_of(m.artistList.begin(), m.artistList.end(),
                                       [&](const std::string& o) { return toLower(o) == toLower(a); });
                if (!dup) m.artistList.push_back(a);
            }
            m.title = rest;
            continue;
        }
        if (!m.version.empty()) break;
        m.version = v;
        m.title   = rest;
    }
    return m;
}

// ── Database import ───────────────────────────────────────────────────────────

namespace {

// Small RAII wrapper so each query below stays a few lines.
class Stmt {
public:
    Stmt(sqlite3* db, const char* sql) {
        if (sqlite3_prepare_v2(db, sql, -1, &m_stmt, nullptr) != SQLITE_OK) m_stmt = nullptr;
    }
    ~Stmt() { if (m_stmt) sqlite3_finalize(m_stmt); }
    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;

    Stmt& text(int i, const std::string& v) {
        if (m_stmt) sqlite3_bind_text(m_stmt, i, v.c_str(), -1, SQLITE_TRANSIENT);
        return *this;
    }
    // Empty string → SQL NULL
    Stmt& optText(int i, const std::string& v) {
        if (!m_stmt) return *this;
        if (v.empty()) sqlite3_bind_null(m_stmt, i);
        else           sqlite3_bind_text(m_stmt, i, v.c_str(), -1, SQLITE_TRANSIENT);
        return *this;
    }
    Stmt& int64(int i, int64_t v) {
        if (m_stmt) sqlite3_bind_int64(m_stmt, i, v);
        return *this;
    }

    bool row()  { return m_stmt && sqlite3_step(m_stmt) == SQLITE_ROW; }
    bool exec() { return m_stmt && sqlite3_step(m_stmt) == SQLITE_DONE; }
    int64_t column64(int c) { return sqlite3_column_int64(m_stmt, c); }

    int64_t singleId() { return row() ? column64(0) : -1; }

private:
    sqlite3_stmt* m_stmt = nullptr;
};

std::string localKey(const std::string& a, const std::string& b = {}, const std::string& c = {}) {
    std::string key = "local:" + toLower(a);
    if (!b.empty()) key += "|" + toLower(b);
    if (!c.empty()) key += "|" + toLower(c);
    return key;
}

void ensureArtist(sqlite3* db, const std::string& key, const std::string& name) {
    Stmt(db, "INSERT OR IGNORE INTO artists (isni, name) VALUES (?, ?)").text(1, key).text(2, name).exec();
}

// Existing tracks matching the file: by ISRC, else by title/version/artist/album.
std::vector<int64_t> findTracks(sqlite3* db, const AudioFileMetadata& m) {
    std::vector<int64_t> ids;
    if (!m.isrc.empty()) {
        Stmt s(db, "SELECT id FROM tracks WHERE recording_id = ? LIMIT 10");
        s.text(1, m.isrc);
        while (s.row()) ids.push_back(s.column64(0));
        if (!ids.empty()) return ids;
    }
    if (m.title.empty()) return ids;

    Stmt s(db, R"(
        SELECT DISTINCT t.id
        FROM tracks t
        JOIN recordings rec ON rec.isrc = t.recording_id
        LEFT JOIN releases rel ON rel.upc = t.release_id
        WHERE LOWER(rec.name) = LOWER(?1)
          AND LOWER(COALESCE(rec.version, '')) = LOWER(?2)
          AND (?3 = '' OR LOWER(COALESCE(rel.name, '')) = LOWER(?3))
          AND (?4 = '' OR EXISTS (
                SELECT 1 FROM recording_artists ra
                JOIN artists a ON a.isni = ra.artist_id
                WHERE ra.recording_id = rec.isrc AND LOWER(a.name) = LOWER(?4)))
        LIMIT 10
    )");
    s.text(1, m.title).text(2, m.version).text(3, m.album)
     .text(4, m.artistList.empty() ? std::string() : m.artistList.front());
    while (s.row()) ids.push_back(s.column64(0));
    return ids;
}

// Create recording / artists / release / track rows from the file's metadata.
int64_t createTrack(sqlite3* db, const AudioFileMetadata& m) {
    if (m.title.empty()) return -1;
    const std::string primary = m.artistList.empty() ? std::string() : m.artistList.front();

    const std::string recordingKey = !m.isrc.empty() ? m.isrc : localKey(primary, m.title, m.version);
    Stmt(db, "INSERT OR IGNORE INTO recordings (isrc, name, version) VALUES (?, ?, ?)")
        .text(1, recordingKey).text(2, m.title).optText(3, m.version).exec();

    for (size_t i = 0; i < m.artistList.size(); ++i) {
        const std::string artistKey = localKey(m.artistList[i]);
        ensureArtist(db, artistKey, m.artistList[i]);
        Stmt(db, "INSERT OR IGNORE INTO recording_artists (recording_id, artist_id, is_primary, number)"
                 " VALUES (?, ?, ?, ?)")
            .text(1, recordingKey).text(2, artistKey)
            .int64(3, i == 0 ? 1 : 0).int64(4, static_cast<int64_t>(i)).exec();
    }

    std::string releaseKey;
    if (!m.album.empty()) {
        const std::string releaseArtist = !m.albumArtist.empty() ? m.albumArtist : primary;
        releaseKey = localKey(releaseArtist, m.album);
        Stmt(db, "INSERT OR IGNORE INTO releases (upc, name, release_date) VALUES (?, ?, ?)")
            .text(1, releaseKey).text(2, m.album).optText(3, m.releaseDate).exec();
        if (!releaseArtist.empty()) {
            ensureArtist(db, localKey(releaseArtist), releaseArtist);
            Stmt(db, "INSERT OR IGNORE INTO release_artists (release_id, artist_id, number, is_primary)"
                     " VALUES (?, ?, 0, 1)")
                .text(1, releaseKey).text(2, localKey(releaseArtist)).exec();
        }
    }

    int64_t trackId = Stmt(db, "SELECT id FROM tracks WHERE recording_id = ? AND release_id IS ? LIMIT 1")
                          .text(1, recordingKey).optText(2, releaseKey).singleId();
    if (trackId >= 0) return trackId;

    if (!Stmt(db, "INSERT INTO tracks (recording_id, release_id, number) VALUES (?, ?, ?)")
             .text(1, recordingKey).optText(2, releaseKey).int64(3, m.trackNumber).exec())
        return -1;
    return sqlite3_last_insert_rowid(db);
}

// Number a matched track from the file's tag when it has no position yet.
void fillTrackNumber(sqlite3* db, int64_t trackId, const AudioFileMetadata& m) {
    if (m.trackNumber <= 0) return;
    Stmt(db, "UPDATE tracks SET number = ? WHERE id = ? AND number = 0 AND release_id IS NOT NULL")
        .int64(1, m.trackNumber).int64(2, trackId).exec();
}

} // namespace

AudioImporter::Result AudioImporter::importFile(sqlite3* db, const std::string& path) {
    Result r;
    if (!db) return r;

    const AudioFileMetadata m = readMetadata(path);

    sqlite3_exec(db, "SAVEPOINT audio_import", nullptr, nullptr, nullptr);

    // files
    Stmt(db, "INSERT OR IGNORE INTO files (path) VALUES (?)").text(1, path).exec();
    r.fileId = Stmt(db, "SELECT id FROM files WHERE path = ?").text(1, path).singleId();
    if (r.fileId < 0) {
        sqlite3_exec(db, "ROLLBACK TO audio_import; RELEASE audio_import", nullptr, nullptr, nullptr);
        return r;
    }

    // audio_files — refresh metadata if the file was imported before
    std::string artists;
    for (const auto& a : m.artistList) artists += (artists.empty() ? "" : ", ") + a;
    r.audioFileId = Stmt(db, "SELECT id FROM audio_files WHERE file_id = ? LIMIT 1").int64(1, r.fileId).singleId();
    if (r.audioFileId >= 0) {
        Stmt(db, "UPDATE audio_files SET name = ?, version = ?, artists = ?, album = ?, duration = ?"
                 " WHERE id = ?")
            .text(1, m.title).optText(2, m.version).text(3, artists).text(4, m.album)
            .int64(5, m.duration).int64(6, r.audioFileId).exec();
    } else if (Stmt(db, "INSERT INTO audio_files (file_id, name, version, artists, album, duration)"
                        " VALUES (?, ?, ?, ?, ?, ?)")
                   .int64(1, r.fileId).text(2, m.title).optText(3, m.version).text(4, artists)
                   .text(5, m.album).int64(6, m.duration).exec()) {
        r.audioFileId = sqlite3_last_insert_rowid(db);
    }

    // playables
    const std::string uri = "file://" + path;
    Stmt(db, "INSERT OR IGNORE INTO playables (uri) VALUES (?)").text(1, uri).exec();
    r.playableId = Stmt(db, "SELECT id FROM playables WHERE uri = ?").text(1, uri).singleId();

    // recording / track
    std::vector<int64_t> trackIds = findTracks(db, m);
    if (trackIds.empty()) {
        int64_t created = createTrack(db, m);
        if (created >= 0) trackIds.push_back(created);
    }

    for (int64_t trackId : trackIds) {
        fillTrackNumber(db, trackId, m);
        if (r.audioFileId >= 0)
            Stmt(db, "INSERT OR IGNORE INTO track_audio_files (track_id, audio_file_id) VALUES (?, ?)")
                .int64(1, trackId).int64(2, r.audioFileId).exec();
        if (r.playableId >= 0)
            Stmt(db, "INSERT OR IGNORE INTO playable_tracks (playable_id, track_id) VALUES (?, ?)")
                .int64(1, r.playableId).int64(2, trackId).exec();
        Stmt(db, "INSERT INTO saves (track_id) SELECT ?1"
                 " WHERE NOT EXISTS (SELECT 1 FROM saves WHERE track_id = ?1 AND deleted_at IS NULL)")
            .int64(1, trackId).exec();
    }
    if (!trackIds.empty()) r.trackId = trackIds.front();

    sqlite3_exec(db, "RELEASE audio_import", nullptr, nullptr, nullptr);
    return r;
}

} // namespace space
