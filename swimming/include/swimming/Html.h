#ifndef SWIMMING_HTML_H
#define SWIMMING_HTML_H

#include <string>
#include <vector>

/// User-agent defaults for every HTML element.
namespace swimming {
    const std::vector<std::string>& allElements();

    enum class DefaultDisplay { None, Block, Inline };

    DefaultDisplay defaultDisplay(const std::string& tag);

    /// UA stylesheet declarations (inline CSS text) for tag.
    const char* uaStyle(const std::string& tag);
}

#endif
