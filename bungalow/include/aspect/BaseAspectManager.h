#pragma once

#include "aspect/Aspect.h"
#include "aspect/AspectManager.h"

#include <shore/aspect_manager.h>

#include <memory>
#include <string>
#include <vector>

namespace space {
namespace aspect {

/// Shore's insertion-ordered id -> aspect registry under the client's method
/// names. Concrete managers extend it and override registerAspect() /
/// unregisterAspect() to wire aspects into the running application (and to
/// undo that again).
template <class T>
class BaseAspectManager : public shore::SHBaseAspectManager<T> {
    using Shore = shore::SHBaseAspectManager<T>;

public:
    /// Takes ownership of aspect and returns it. An aspect with the same id
    /// is unregistered first.
    virtual T* registerAspect(std::unique_ptr<T> aspect) { return Shore::Register(std::move(aspect)); }
    /// Unregisters and destroys the aspect with this id, if any.
    virtual void unregisterAspect(const std::string& id) { Shore::Unregister(id); }
    T* get(const std::string& id) const { return Shore::Get(id); }
    /// Every registered aspect, in registration order.
    std::vector<T*> all() const { return Shore::All(); }

    // Shore's spelling ends up in the overridable functions above.
    T* Register(std::unique_ptr<T> aspect) final { return registerAspect(std::move(aspect)); }
    void Unregister(const std::string& id) final { unregisterAspect(id); }
};

} // namespace aspect
} // namespace space
