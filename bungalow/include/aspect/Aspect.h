#pragma once

#include <string>

namespace space {
namespace aspect {

class AspectManagerBase;

/// Anything pluggable — a feature, a concept, a search provider, … — has a
/// stable identity and is told when it is registered with its manager.
class Aspect {
public:
    virtual ~Aspect() = default;

    virtual std::string getId() const = 0;
    virtual std::string getName() const = 0;

    /// Called once by the manager the aspect was registered with.
    virtual void onRegister(AspectManagerBase& /*manager*/) {}
};

} // namespace aspect
} // namespace space
