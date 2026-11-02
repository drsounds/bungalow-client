#ifndef SWIMMING_HTML_DOCUMENT_H
#define SWIMMING_HTML_DOCUMENT_H

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

/// A forgiving HTML parser, enough to mount a page's body and find its
/// scripts: elements, attributes, text, comments, void and raw-text
/// elements, and character references. Unknown or mismatched end tags are
/// ignored rather than repaired.
namespace swimming {
    struct HtmlNode {
        std::string name; ///< lowercase tag name; empty for text
        std::vector<std::pair<std::string, std::string>> attributes;
        std::string text; ///< text nodes; the content of raw-text elements (script, style, ...)
        std::vector<std::unique_ptr<HtmlNode>> children;

        bool isText() const { return name.empty(); }
        std::optional<std::string> attribute(const std::string& key) const;
        /// Text content of the node and its descendants.
        std::string textContent() const;
    };

    /// The document root: an unnamed element holding the top-level nodes.
    std::unique_ptr<HtmlNode> parseHtml(const std::string& html);
}

#endif
