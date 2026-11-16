#pragma once

#include <shore/aspect.h>

#include <string>

// Aspects live in Shore, the client's architecture foundation (shore::SHAspect
// and its managers). This header keeps the client's own spelling of them:
// getId()/getName() instead of Id()/Name().
namespace space {
namespace aspect {

/// Untyped base of every aspect manager, handed to Aspect::onRegister().
using AspectManagerBase = shore::SHAspectManagerBase;

/// Gives a Shore aspect class (shore::SHAspect, SHFeature, SHConcept,
/// SHService) the client's method names. The Shore virtuals are final and
/// forward to them, so a class written against either spelling works with
/// Shore's managers.
template <class Base>
class AspectNames : public Base {
public:
    using Base::Base;

    virtual std::string getId() const = 0;
    virtual std::string getName() const = 0;

    /// Called once by the manager the aspect was registered with.
    virtual void onRegister(AspectManagerBase& /*manager*/) {}

    std::string Id() const final { return getId(); }
    std::string Name() const final { return getName(); }
    void OnRegister(shore::SHAspectManagerBase& manager) final { onRegister(manager); }
};

/// Anything pluggable — a feature, a concept, a search provider, … — has a
/// stable identity and is told when it is registered with its manager.
class Aspect : public AspectNames<shore::SHAspect> {};

} // namespace aspect
} // namespace space
