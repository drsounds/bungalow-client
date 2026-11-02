#pragma once

#include <memory>
#include <string>
#include <vector>

namespace space {
namespace aspect {

/// Untyped base of every aspect manager, handed to Aspect::onRegister().
class AspectManagerBase {
public:
    virtual ~AspectManagerBase() = default;
};

/// A registry of one kind of aspect, keyed by Aspect::getId(). Managers own
/// the aspects registered with them.
template <class T>
class AspectManager : public AspectManagerBase {
public:
    /// Takes ownership of aspect and returns it. An aspect with the same id
    /// is unregistered first.
    virtual T* registerAspect(std::unique_ptr<T> aspect) = 0;
    /// Unregisters and destroys the aspect with this id, if any.
    virtual void unregisterAspect(const std::string& id) = 0;
    virtual T* get(const std::string& id) const = 0;
    /// Every registered aspect, in registration order.
    virtual std::vector<T*> all() const = 0;
};

} // namespace aspect
} // namespace space
