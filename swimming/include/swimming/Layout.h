#ifndef SWIMMING_LAYOUT_H
#define SWIMMING_LAYOUT_H

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "swimming/Style.h"

/// Shadow tree -> computed styles -> Yoga layout. No DOM is built.
namespace swimming {
    /// A node of the shadow tree React's reconciler builds.
    struct Node {
        std::string kind;                ///< tag name, "#text" or "root"
        std::optional<std::string> text; ///< text nodes only
        std::string style;               ///< inline CSS text, e.g. "width: 10px; color: red"
        nlohmann::json props = nlohmann::json::object();
        std::vector<std::uint64_t> children;
        std::optional<std::uint64_t> parent;
    };
    using NodeMap = std::unordered_map<std::uint64_t, Node>;

    /// The inherited and painted part of an element's style.
    struct Computed {
        Rgba color{0, 0, 0, 255};
        float fontSize = 16;
        bool bold = false;
        std::optional<Rgba> background;
        Rgba borderColor{0, 0, 0, 255};
        float borderWidth = 0;
        float radius = 0;
        float opacity = 1;
    };

    /// A laid out box, positioned relative to its parent box.
    struct LayoutBox {
        std::optional<std::uint64_t> id; ///< none for text and anonymous boxes
        std::string tag;                 ///< tag name, "#text" or "#anon"
        std::optional<std::string> text;
        Computed computed;
        float x = 0, y = 0, w = 0, h = 0;
        std::vector<LayoutBox> children;
    };

    struct LaidOut {
        LayoutBox root;

        /// {id, type, x, y, width, height, children} for every box.
        nlohmann::json summary() const;

        /// Deepest real element containing the point; text and anonymous
        /// boxes resolve to their element.
        std::optional<std::uint64_t> hitTest(float x, float y) const;
    };

    /// Lays out the tree under root in a width x height viewport.
    LaidOut layout(const NodeMap& nodes, std::uint64_t root, float width, float height);
}

#endif
