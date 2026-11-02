#ifndef SWIMMING_ENGINE_H
#define SWIMMING_ENGINE_H

#include <cstdint>
#include <optional>
#include <string>

#include "swimming/Layout.h"

/// The shadow tree and native layout engine.
///
/// The React reconciler drives this through a small command API (create /
/// append / insert / remove / update). There is no DOM: inline CSS is resolved
/// and laid out with Yoga directly from the shadow tree.
namespace swimming {
    /// Id of the implicit root container.
    constexpr std::uint64_t kRoot = 0;

    class Engine {
    public:
        Engine();

        std::uint64_t createNode(const std::string& tag, const std::string& styleCss, const std::string& propsJson);
        std::uint64_t createText(const std::string& text);
        void setText(std::uint64_t id, const std::string& text);
        void updateNode(std::uint64_t id, const std::string& styleCss, const std::string& propsJson);

        void appendChild(std::uint64_t parent, std::uint64_t child) { insertBefore(parent, child, std::nullopt); }
        /// Inserts child into parent before `before` (or at the end), detaching it from any old parent.
        void insertBefore(std::uint64_t parent, std::uint64_t child, std::optional<std::uint64_t> before);
        /// Removes child from parent and frees the whole subtree.
        void removeChild(std::uint64_t parent, std::uint64_t child);

        size_t nodeCount() const { return m_nodes.size(); }
        const Node* node(std::uint64_t id) const;
        std::optional<std::uint64_t> parent(std::uint64_t id) const;

        void computeLayout(float width, float height);
        /// The layout of the last computeLayout, or null before the first one.
        const LaidOut* layout() const { return m_laidOut ? &*m_laidOut : nullptr; }
        float width() const { return m_width; }
        float height() const { return m_height; }

        /// Deepest element under the point (never the root container).
        std::optional<std::uint64_t> hitTest(float x, float y) const;

        /// Whether the element has an on<Event> handler flagged by the JS side.
        bool hasHandler(std::uint64_t id, const std::string& prop) const;

    private:
        std::uint64_t alloc(Node node);
        void detach(std::uint64_t child);

        NodeMap m_nodes;
        std::uint64_t m_nextId = 1;
        float m_width = 800;
        float m_height = 600;
        std::optional<LaidOut> m_laidOut;
    };
}

#endif
