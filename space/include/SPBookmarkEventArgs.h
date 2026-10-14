#pragma once
#include "SPEventArgs.h"
#include <string>

namespace space {

class SPBookmarkEventArgs : public SPEventArgs {
    std::string m_uri;
    std::string m_title;
public:
    SPBookmarkEventArgs(const std::string& uri, const std::string& title)
        : m_uri(uri), m_title(title) {}
    const std::string& getUri()   const { return m_uri; }
    const std::string& getTitle() const { return m_title; }
    std::string getType() override { return "SPBookmarkEventArgs"; }
};

} // namespace space
