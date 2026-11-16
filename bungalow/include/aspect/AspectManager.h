#pragma once

#include "aspect/Aspect.h"

#include <shore/aspect_manager.h>

namespace space {
namespace aspect {

/// A registry of one kind of aspect, keyed by its id. Managers own the
/// aspects registered with them. Defined by Shore.
template <class T>
using AspectManager = shore::SHAspectManager<T>;

} // namespace aspect
} // namespace space
