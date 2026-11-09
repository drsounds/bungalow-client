#include "service/MusicService.h"

namespace space {
namespace service {

std::optional<MusicService::Candidate> MusicService::lookupByTitleArtist(const std::string&,
                                                                         const std::string&) {
    return std::nullopt;
}

std::vector<MusicService::Candidate> MusicService::findCandidates(const std::string& isrc,
                                                                  const std::string& title,
                                                                  const std::string& artist) {
    if (!isrc.empty()) {
        if (auto c = lookup(isrc)) {
            c->byIsrc = true;
            return {*c};
        }
    }
    if (!title.empty()) {
        if (auto c = lookupByTitleArtist(title, artist)) {
            c->byIsrc = false;
            return {*c};
        }
    }
    return {};
}

void MusicService::loadByIsrc(const std::string& isrc) {
    auto c = lookup(isrc);
    if (c && !c->uri.empty())
        loadUri(c->uri);
    else
        fireError("Couldn't find " + isrc + " on " + getName());
}

void MusicService::loadByTitleArtist(const std::string& title, const std::string& artist) {
    auto c = lookupByTitleArtist(title, artist);
    if (c && !c->uri.empty())
        loadUri(c->uri);
    else
        fireError("Couldn't find \"" + title + "\" on " + getName());
}

} // namespace service
} // namespace space
