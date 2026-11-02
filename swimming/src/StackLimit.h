#ifndef SWIMMING_STACK_LIMIT_H
#define SWIMMING_STACK_LIMIT_H

#include <cstdint>
#include <optional>

namespace swimming {
    /// Room left below the engine's stack limit for native frames (the
    /// engine's own, and the C++ natives JS calls).
    constexpr std::uintptr_t kStackMargin = 128 * 1024;

    /// The lowest address the current thread's stack may reach plus
    /// kStackMargin: where a JS engine must stop recursing so that deep JS
    /// recursion throws a RangeError instead of overflowing the real stack.
    /// nullopt where the stack bounds are unknown.
    std::optional<std::uintptr_t> stackLimit();
}

#endif
