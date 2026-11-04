#include "playlist/Upsl.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace space {
namespace playlist {
namespace {

const char kAlphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

// The canonical prefix first; the others are earlier spellings (RFC-0001 §5).
const char* const kPrefixes[] = {
    Upsl::kPrefix, "bungalow:federated:user:", "spacify:federated:user:",
};

const std::string kPlaylistMarker = ":playlist:";
const std::string kDescriptionMarker = ":description:";
const std::string kImageMarker = ":image:";
const std::string kUrisMarker = ":uris:";
const std::string kSigMarker = ":sig:";

// The rest of uri after its federated prefix, or nullopt.
std::optional<std::string> stripPrefix(const std::string& uri) {
    for (const char* prefix : kPrefixes) {
        if (uri.rfind(prefix, 0) == 0) return uri.substr(std::strlen(prefix));
    }
    return std::nullopt;
}

bool isHandleChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.' || c == '-';
}

} // namespace

// ── Base62 ────────────────────────────────────────────────────────────────────

std::string base62::encode(const std::string& bytes) {
    size_t zeros = 0;
    while (zeros < bytes.size() && bytes[zeros] == '\0') ++zeros;

    // Repeated division of the big-endian number by 62.
    std::vector<unsigned char> number(bytes.begin() + zeros, bytes.end());
    std::string digits;
    while (!number.empty()) {
        std::vector<unsigned char> quotient;
        int remainder = 0;
        for (unsigned char byte : number) {
            const int value = remainder * 256 + byte;
            const int q = value / 62;
            remainder = value % 62;
            if (!quotient.empty() || q != 0) quotient.push_back(static_cast<unsigned char>(q));
        }
        digits += kAlphabet[remainder];
        number = std::move(quotient);
    }
    digits.append(zeros, '0');
    std::reverse(digits.begin(), digits.end());
    return digits;
}

std::optional<std::string> base62::decode(const std::string& text) {
    size_t zeros = 0;
    while (zeros < text.size() && text[zeros] == '0') ++zeros;

    // number = number * 62 + digit, big-endian.
    std::vector<unsigned char> number;
    for (size_t i = zeros; i < text.size(); ++i) {
        const char* found = std::strchr(kAlphabet, text[i]);
        if (found == nullptr || text[i] == '\0') return std::nullopt;
        int carry = static_cast<int>(found - kAlphabet);
        for (auto it = number.rbegin(); it != number.rend(); ++it) {
            const int value = *it * 62 + carry;
            *it = static_cast<unsigned char>(value & 0xFF);
            carry = value >> 8;
        }
        while (carry > 0) {
            number.insert(number.begin(), static_cast<unsigned char>(carry & 0xFF));
            carry >>= 8;
        }
    }
    std::string out(zeros, '\0');
    out.append(number.begin(), number.end());
    return out;
}

// ── Upsl ──────────────────────────────────────────────────────────────────────

bool Upsl::isUpsl(const std::string& uri) {
    return stripPrefix(uri).has_value();
}

std::string Upsl::canonicalUri(const std::string& uri) {
    const auto rest = stripPrefix(uri);
    return rest ? kPrefix + *rest : uri;
}

std::optional<Upsl> Upsl::parse(const std::string& uri) {
    const auto rest = stripPrefix(uri);
    if (!rest) return std::nullopt;

    const size_t playlistAt = rest->find(kPlaylistMarker);
    if (playlistAt == std::string::npos) return std::nullopt;
    Upsl upsl;
    upsl.handle = rest->substr(0, playlistAt);
    const std::string afterPlaylist = rest->substr(playlistAt + kPlaylistMarker.size());

    // The playlist id is the next ':'-delimited token (a UUID has no colons).
    const size_t idEnd = afterPlaylist.find(':');
    upsl.playlistId = afterPlaylist.substr(0, idEnd);
    if (upsl.playlistId.empty()) return std::nullopt;
    if (idEnd == std::string::npos) return upsl;

    std::string body = afterPlaylist.substr(idEnd + 1);
    const size_t sigAt = body.rfind(kSigMarker);
    if (sigAt != std::string::npos) body.resize(sigAt);
    // A bare signature after the id still leaves a hosted reference.
    if (body.rfind("sig:", 0) == 0) return upsl;

    const size_t descriptionAt = body.find(kDescriptionMarker);
    if (descriptionAt == std::string::npos) return upsl;
    upsl.embedded = true;

    const auto name = base62::decode(body.substr(0, descriptionAt));
    if (!name) return std::nullopt;
    upsl.name = *name;

    const std::string afterDescription = body.substr(descriptionAt + kDescriptionMarker.size());
    const size_t descriptionEnd = std::min(afterDescription.find(':'), afterDescription.size());
    const auto description = base62::decode(afterDescription.substr(0, descriptionEnd));
    if (!description) return std::nullopt;
    upsl.description = *description;

    // Unknown :key:value segments may sit before :uris:, which is last.
    const size_t urisAt = body.find(kUrisMarker);
    if (urisAt == std::string::npos) return upsl;
    const std::string list = body.substr(urisAt + kUrisMarker.size());
    size_t start = 0;
    while (start <= list.size()) {
        const size_t comma = std::min(list.find(',', start), list.size());
        const std::string leaf = list.substr(start, comma - start);
        if (!leaf.empty()) {
            const auto ref = base62::decode(leaf);
            if (!ref) return std::nullopt;
            upsl.refs.push_back(*ref);
        }
        start = comma + 1;
    }
    return upsl;
}

std::string Upsl::hostedUri(const std::string& handle, const std::string& playlistId) {
    return kPrefix + handle + kPlaylistMarker + playlistId;
}

std::string Upsl::embeddedUri() const {
    std::string uri = hostedUri(handle, playlistId) + ":" + base62::encode(name) + kDescriptionMarker +
                      base62::encode(description) + kUrisMarker;
    for (size_t i = 0; i < refs.size(); ++i) {
        if (i > 0) uri += ",";
        uri += base62::encode(refs[i]);
    }
    return uri;
}

bool Upsl::isValidHandle(const std::string& handle) {
    if (handle.size() < 4 || handle[0] != '@') return false;
    const size_t at = handle.find('@', 1);
    if (at == std::string::npos || at == 1 || at + 1 == handle.size()) return false;
    for (size_t i = 1; i < handle.size(); ++i) {
        if (i == at) continue;
        // The domain has no '_'.
        if (!isHandleChar(handle[i]) || (i > at && handle[i] == '_')) return false;
    }
    return true;
}

std::string Upsl::defaultHandle() {
    const char* user = std::getenv("USERNAME");
    if (user == nullptr || *user == '\0') user = std::getenv("USER");
    std::string localpart;
    for (const char* c = user; c != nullptr && *c != '\0'; ++c)
        if (isHandleChar(*c)) localpart += static_cast<char>(std::tolower(static_cast<unsigned char>(*c)));
    if (localpart.empty()) localpart = "me";
    return "@" + localpart + "@localhost";
}

std::string percentDecode(const std::string& text) {
    std::string out;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '%' && i + 2 < text.size() && std::isxdigit(static_cast<unsigned char>(text[i + 1])) &&
            std::isxdigit(static_cast<unsigned char>(text[i + 2]))) {
            out += static_cast<char>(std::strtol(text.substr(i + 1, 2).c_str(), nullptr, 16));
            i += 2;
        } else {
            out += text[i];
        }
    }
    return out;
}

} // namespace playlist
} // namespace space
