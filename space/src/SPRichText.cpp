#include "SPRichText.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <utility>

// Parsing, layout and hit testing. Painting is in SPRichTextPaint.cpp so that
// this file needs nothing but the font and rectangle types.

namespace space {
namespace {

    struct Tag {
        std::string name;
        bool closing = false;
        std::map<std::string, std::string> attributes;
    };

    std::string lower(std::string value) {
        for (char& c : value)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return value;
    }

    bool isSpace(char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }

    void appendUtf8(std::string& out, unsigned long code) {
        if (code < 0x80) {
            out += static_cast<char>(code);
        } else if (code < 0x800) {
            out += static_cast<char>(0xC0 | (code >> 6));
            out += static_cast<char>(0x80 | (code & 0x3F));
        } else if (code < 0x10000) {
            out += static_cast<char>(0xE0 | (code >> 12));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (code & 0x3F));
        } else if (code < 0x110000) {
            out += static_cast<char>(0xF0 | (code >> 18));
            out += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (code & 0x3F));
        }
    }

    // Decodes the entity starting at text[pos] ('&'). Returns the position
    // after it, or pos when there is no entity there.
    size_t decodeEntity(const std::string& text, size_t pos, std::string& out) {
        const size_t end = text.find(';', pos);
        if (end == std::string::npos || end - pos > 10 || end == pos + 1)
            return pos;
        const std::string name = text.substr(pos + 1, end - pos - 1);
        if (name == "amp") out += '&';
        else if (name == "lt") out += '<';
        else if (name == "gt") out += '>';
        else if (name == "quot") out += '"';
        else if (name == "apos") out += '\'';
        else if (name == "nbsp") out += "\xC2\xA0";
        else if (name[0] == '#' && name.size() > 1) {
            const bool hex = name[1] == 'x' || name[1] == 'X';
            const std::string digits = name.substr(hex ? 2 : 1);
            if (digits.empty())
                return pos;
            char* stop = nullptr;
            const unsigned long code = std::strtoul(digits.c_str(), &stop, hex ? 16 : 10);
            if (stop == nullptr || *stop != '\0' || code == 0 || code >= 0x110000)
                return pos;
            appendUtf8(out, code);
        } else {
            return pos;
        }
        return end + 1;
    }

    std::string decodeEntities(const std::string& text) {
        std::string out;
        for (size_t i = 0; i < text.size();) {
            if (text[i] == '&') {
                const size_t next = decodeEntity(text, i, out);
                if (next != i) {
                    i = next;
                    continue;
                }
            }
            out += text[i++];
        }
        return out;
    }

    // Reads the tag starting at text[pos] ('<'). Returns the position after
    // its '>', or pos when what follows is not a well-formed tag.
    size_t readTag(const std::string& text, size_t pos, Tag& tag) {
        size_t i = pos + 1;
        if (i < text.size() && text[i] == '/') {
            tag.closing = true;
            ++i;
        }
        const size_t nameStart = i;
        while (i < text.size() && std::isalnum(static_cast<unsigned char>(text[i])))
            ++i;
        if (i == nameStart || !std::isalpha(static_cast<unsigned char>(text[nameStart])))
            return pos;
        tag.name = lower(text.substr(nameStart, i - nameStart));

        while (true) {
            while (i < text.size() && isSpace(text[i]))
                ++i;
            if (i >= text.size())
                return pos;
            if (text[i] == '>')
                return i + 1;
            if (text[i] == '/') {
                if (i + 1 < text.size() && text[i + 1] == '>')
                    return i + 2;
                return pos;
            }
            const size_t attrStart = i;
            while (i < text.size() && (std::isalnum(static_cast<unsigned char>(text[i])) ||
                                       text[i] == '-' || text[i] == '_' || text[i] == ':'))
                ++i;
            if (i == attrStart)
                return pos;
            const std::string name = lower(text.substr(attrStart, i - attrStart));
            while (i < text.size() && isSpace(text[i]))
                ++i;
            std::string value;
            if (i < text.size() && text[i] == '=') {
                ++i;
                while (i < text.size() && isSpace(text[i]))
                    ++i;
                if (i >= text.size())
                    return pos;
                if (text[i] == '"' || text[i] == '\'') {
                    const char quote = text[i];
                    const size_t close = text.find(quote, i + 1);
                    if (close == std::string::npos)
                        return pos;
                    value = text.substr(i + 1, close - i - 1);
                    i = close + 1;
                } else {
                    const size_t valueStart = i;
                    while (i < text.size() && !isSpace(text[i]) && text[i] != '>' &&
                           !(text[i] == '/' && i + 1 < text.size() && text[i + 1] == '>'))
                        ++i;
                    value = text.substr(valueStart, i - valueStart);
                }
            }
            tag.attributes[name] = decodeEntities(value);
        }
    }

    std::string attribute(const Tag& tag, const char* first, const char* second = nullptr) {
        auto it = tag.attributes.find(first);
        if (it != tag.attributes.end() && !it->second.empty())
            return it->second;
        if (second != nullptr) {
            it = tag.attributes.find(second);
            if (it != tag.attributes.end())
                return it->second;
        }
        return std::string();
    }

    int pixels(const Tag& tag, const char* name) {
        const std::string value = attribute(tag, name);
        const int number = value.empty() ? 0 : std::atoi(value.c_str());
        return std::clamp(number, 0, 4096);
    }

    std::string fontKey(const SPFontStyle& font) {
        return font.family + '/' + std::to_string(font.height) + '/' +
               std::to_string(font.weight) + (font.italic ? "/i" : "/n");
    }

} // namespace

std::vector<SPRichTextRun> SPRichText::parse(const std::string& markup) {
    std::vector<SPRichTextRun> runs;
    std::string text;
    int bold = 0;
    int italic = 0;
    int underline = 0;
    std::vector<std::string> links;

    auto style = [&](SPRichTextRun& run) {
        run.bold = bold > 0;
        run.italic = italic > 0;
        run.underline = underline > 0;
        if (!links.empty())
            run.uri = links.back();
    };
    auto flush = [&] {
        if (text.empty())
            return;
        SPRichTextRun run;
        run.kind = SPRichTextRun::Kind::Text;
        run.text = std::move(text);
        text.clear();
        style(run);
        runs.push_back(std::move(run));
    };
    auto lineBreak = [&] {
        flush();
        SPRichTextRun run;
        run.kind = SPRichTextRun::Kind::LineBreak;
        runs.push_back(std::move(run));
    };

    for (size_t i = 0; i < markup.size();) {
        const char c = markup[i];
        if (c == '&') {
            const size_t next = decodeEntity(markup, i, text);
            if (next != i) {
                i = next;
                continue;
            }
        } else if (c == '\r') {
            ++i;
            continue;
        } else if (c == '\n') {
            lineBreak();
            ++i;
            continue;
        } else if (c == '<') {
            Tag tag;
            const size_t next = readTag(markup, i, tag);
            bool known = next != i;
            if (known) {
                const std::string& name = tag.name;
                if (name == "b" || name == "strong") {
                    flush();
                    bold = tag.closing ? std::max(0, bold - 1) : bold + 1;
                } else if (name == "i" || name == "em") {
                    flush();
                    italic = tag.closing ? std::max(0, italic - 1) : italic + 1;
                } else if (name == "u") {
                    flush();
                    underline = tag.closing ? std::max(0, underline - 1) : underline + 1;
                } else if (name == "a" || name == "link") {
                    flush();
                    if (!tag.closing)
                        links.push_back(name == "a" ? attribute(tag, "href", "uri")
                                                    : attribute(tag, "uri", "href"));
                    else if (!links.empty())
                        links.pop_back();
                } else if (name == "br") {
                    if (!tag.closing)
                        lineBreak();
                } else if (name == "img") {
                    if (!tag.closing) {
                        flush();
                        SPRichTextRun run;
                        run.kind = SPRichTextRun::Kind::Image;
                        run.src = attribute(tag, "src");
                        run.width = pixels(tag, "width");
                        run.height = pixels(tag, "height");
                        style(run);
                        // The image's own uri wins over an enclosing link.
                        const std::string uri = attribute(tag, "uri", "href");
                        if (!uri.empty())
                            run.uri = uri;
                        runs.push_back(std::move(run));
                    }
                } else {
                    known = false;
                }
            }
            if (known) {
                i = next;
                continue;
            }
        }
        text += c;
        ++i;
    }
    flush();
    return runs;
}

std::string SPRichText::escape(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (const char c : text) {
        switch (c) {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        case '\'': escaped += "&apos;"; break;
        default: escaped += c;
        }
    }
    return escaped;
}

std::string SPRichText::wrapLink(const std::string& uri, const std::string& markup) {
    return "<link uri=\"" + escape(uri) + "\">" + markup + "</link>";
}

std::string SPRichText::link(const std::string& uri, const std::string& text) {
    return wrapLink(uri, escape(text));
}

std::string SPRichText::plainText(const std::string& markup) {
    return SPRichText(markup).getPlainText();
}

void SPRichText::setMarkup(const std::string& markup) {
    if (markup == m_markup && (!m_runs.empty() || markup.empty()))
        return;
    m_markup = markup;
    m_runs = parse(markup);
    m_hasLinks = std::any_of(m_runs.begin(), m_runs.end(),
                             [](const SPRichTextRun& run) { return !run.uri.empty(); });
    m_fragments.clear();
    m_width = 0;
    m_height = 0;
    m_laidOut = false;
}

std::string SPRichText::getPlainText() const {
    std::string plain;
    for (const auto& run : m_runs) {
        if (run.kind == SPRichTextRun::Kind::Text)
            plain += run.text;
        else if (run.kind == SPRichTextRun::Kind::LineBreak)
            plain += '\n';
    }
    return plain;
}

SPFontStyle SPRichText::fontFor(const SPRichTextRun& run, const SPFontStyle& base) {
    SPFontStyle font = base;
    if (run.bold)
        font.weight = 700;
    if (run.italic)
        font.italic = true;
    // Underlines are drawn as lines, so they look the same on every backend.
    font.underline = false;
    return font;
}

void SPRichText::layout(const SPFontStyle& font, const Measure& measure) {
    const std::string key = fontKey(font);
    if (m_laidOut && key == m_layoutFont)
        return;
    m_fragments.clear();
    m_width = 0;
    m_height = 0;
    m_layoutFont = key;
    m_laidOut = true;
    if (m_runs.empty() || !measure)
        return;

    const int lineHeight = std::max(1, measure("Ag", font).height);
    int x = 0;
    int y = 0;
    int height = 0;      // of the line being filled
    size_t lineStart = 0; // its first fragment

    auto endLine = [&] {
        height = std::max(height, lineHeight);
        for (size_t i = lineStart; i < m_fragments.size(); ++i)
            m_fragments[i].y = y + (height - m_fragments[i].height) / 2;
        m_width = std::max(m_width, x);
        y += height;
        x = 0;
        height = 0;
        lineStart = m_fragments.size();
    };

    for (size_t i = 0; i < m_runs.size(); ++i) {
        const SPRichTextRun& run = m_runs[i];
        SPRichTextFragment fragment;
        fragment.run = static_cast<int>(i);
        fragment.x = x;
        switch (run.kind) {
        case SPRichTextRun::Kind::LineBreak:
            endLine();
            continue;
        case SPRichTextRun::Kind::Text: {
            const sp_rectangle size = measure(run.text, fontFor(run, font));
            fragment.width = std::max(0, size.width);
            fragment.height = std::max(1, size.height);
            break;
        }
        case SPRichTextRun::Kind::Image:
            fragment.width = run.width > 0 ? run.width : (run.height > 0 ? run.height : lineHeight);
            fragment.height = run.height > 0 ? run.height : (run.width > 0 ? run.width : lineHeight);
            break;
        }
        x += fragment.width;
        height = std::max(height, fragment.height);
        m_fragments.push_back(fragment);
    }
    // A trailing line break does not open another line.
    if (lineStart < m_fragments.size() || m_runs.back().kind != SPRichTextRun::Kind::LineBreak)
        endLine();
    m_height = y;
}

std::string SPRichText::uriAt(int x, int y) const {
    if (!m_hasLinks)
        return std::string();
    for (const auto& fragment : m_fragments) {
        if (x < fragment.x || x >= fragment.x + fragment.width ||
            y < fragment.y || y >= fragment.y + fragment.height)
            continue;
        return m_runs[static_cast<size_t>(fragment.run)].uri;
    }
    return std::string();
}

} // namespace space
