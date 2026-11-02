#include "swimming/Engine.h"

#include <algorithm>

namespace swimming {
    namespace {
        nlohmann::json parseProps(const std::string& json) {
            nlohmann::json props = nlohmann::json::parse(json, nullptr, false);
            return props.is_object() ? props : nlohmann::json::object();
        }
    }

    Engine::Engine() {
        Node root;
        root.kind = "root";
        m_nodes.emplace(kRoot, std::move(root));
    }

    std::uint64_t Engine::alloc(Node node) {
        const std::uint64_t id = m_nextId++;
        m_nodes.emplace(id, std::move(node));
        return id;
    }

    std::uint64_t Engine::createNode(const std::string& tag, const std::string& styleCss, const std::string& propsJson) {
        Node node;
        node.kind = tag;
        node.style = styleCss;
        node.props = parseProps(propsJson);
        return alloc(std::move(node));
    }

    std::uint64_t Engine::createText(const std::string& text) {
        Node node;
        node.kind = "#text";
        node.text = text;
        return alloc(std::move(node));
    }

    void Engine::setText(std::uint64_t id, const std::string& text) {
        const auto it = m_nodes.find(id);
        if (it != m_nodes.end()) it->second.text = text;
    }

    void Engine::updateNode(std::uint64_t id, const std::string& styleCss, const std::string& propsJson) {
        const auto it = m_nodes.find(id);
        if (it == m_nodes.end()) return;
        it->second.style = styleCss;
        it->second.props = parseProps(propsJson);
    }

    void Engine::insertBefore(std::uint64_t parent, std::uint64_t child, std::optional<std::uint64_t> before) {
        if (parent == child || !m_nodes.count(parent) || !m_nodes.count(child)) return;
        detach(child);
        auto& children = m_nodes[parent].children;
        auto at = before ? std::find(children.begin(), children.end(), *before) : children.end();
        children.insert(at, child);
        m_nodes[child].parent = parent;
    }

    void Engine::detach(std::uint64_t child) {
        const auto it = m_nodes.find(child);
        if (it == m_nodes.end()) return;
        if (it->second.parent) {
            const auto old = m_nodes.find(*it->second.parent);
            if (old != m_nodes.end()) {
                auto& siblings = old->second.children;
                siblings.erase(std::remove(siblings.begin(), siblings.end(), child), siblings.end());
            }
        }
        it->second.parent.reset();
    }

    void Engine::removeChild(std::uint64_t parent, std::uint64_t child) {
        if (this->parent(child) != parent) return;
        detach(child);
        std::vector<std::uint64_t> stack{child};
        while (!stack.empty()) {
            const std::uint64_t id = stack.back();
            stack.pop_back();
            const auto it = m_nodes.find(id);
            if (it == m_nodes.end()) continue;
            stack.insert(stack.end(), it->second.children.begin(), it->second.children.end());
            m_nodes.erase(it);
        }
    }

    const Node* Engine::node(std::uint64_t id) const {
        const auto it = m_nodes.find(id);
        return it == m_nodes.end() ? nullptr : &it->second;
    }

    std::optional<std::uint64_t> Engine::parent(std::uint64_t id) const {
        const Node* n = node(id);
        return n ? n->parent : std::nullopt;
    }

    void Engine::computeLayout(float width, float height) {
        m_width = width;
        m_height = height;
        m_laidOut = swimming::layout(m_nodes, kRoot, width, height);
    }

    std::optional<std::uint64_t> Engine::hitTest(float x, float y) const {
        if (!m_laidOut) return std::nullopt;
        const auto id = m_laidOut->hitTest(x, y);
        if (id == kRoot) return std::nullopt;
        return id;
    }

    bool Engine::hasHandler(std::uint64_t id, const std::string& prop) const {
        const Node* n = node(id);
        if (!n) return false;
        const auto it = n->props.find(prop);
        return it != n->props.end() && it->is_boolean() && it->get<bool>();
    }
}
