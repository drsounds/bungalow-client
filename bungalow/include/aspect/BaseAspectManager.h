#pragma once

#include "aspect/Aspect.h"
#include "aspect/AspectManager.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace space {
namespace aspect {

/// Insertion-ordered id -> aspect registry. Concrete managers extend it and
/// override registerAspect() / unregisterAspect() to wire aspects into the
/// running application (and to undo that again).
template <class T>
class BaseAspectManager : public AspectManager<T> {
public:
    T* registerAspect(std::unique_ptr<T> aspect) override {
        if (!aspect) return nullptr;
        // Virtual, so a subclass can undo whatever the old aspect added.
        if (find(aspect->getId()) != m_nodes.end()) this->unregisterAspect(aspect->getId());
        T* raw = aspect.get();
        m_nodes.push_back(std::move(aspect));
        raw->onRegister(*this);
        return raw;
    }

    void unregisterAspect(const std::string& id) override {
        auto it = find(id);
        if (it != m_nodes.end()) m_nodes.erase(it);
    }

    T* get(const std::string& id) const override {
        auto it = find(id);
        return it == m_nodes.end() ? nullptr : it->get();
    }

    std::vector<T*> all() const override {
        std::vector<T*> result;
        result.reserve(m_nodes.size());
        for (const auto& node : m_nodes) result.push_back(node.get());
        return result;
    }

private:
    using Nodes = std::vector<std::unique_ptr<T>>;

    typename Nodes::const_iterator find(const std::string& id) const {
        return std::find_if(m_nodes.begin(), m_nodes.end(),
                            [&](const std::unique_ptr<T>& node) { return node->getId() == id; });
    }

    Nodes m_nodes;
};

} // namespace aspect
} // namespace space
