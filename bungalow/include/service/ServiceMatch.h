#pragma once

#include "service/MusicService.h"

#include <string>

namespace space {
namespace service {

/// One entry of the "Play with…" chooser: a MusicService plus one concrete
/// candidate it offered. A service may contribute several.
struct ServiceMatch {
    MusicService*           service = nullptr; // owned by the ServiceManager
    MusicService::Candidate candidate;

    /// "Local Files — Song (Artist)", for chooser UI.
    std::string label() const {
        std::string text = service ? service->getName() : std::string();
        if (!candidate.title.empty()) {
            text += " \xE2\x80\x94 " + candidate.title; // em dash
            if (!candidate.artist.empty()) text += " (" + candidate.artist + ")";
        }
        return text;
    }
};

} // namespace service
} // namespace space
