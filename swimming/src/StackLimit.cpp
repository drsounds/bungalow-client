#include "StackLimit.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#elif defined(__APPLE__)
#include <pthread.h>
#elif defined(__linux__)
#include <pthread.h>
#endif

namespace swimming {
    std::optional<std::uintptr_t> stackLimit() {
#if defined(_WIN32)
        ULONG_PTR low = 0, high = 0;
        GetCurrentThreadStackLimits(&low, &high);
        return static_cast<std::uintptr_t>(low) + kStackMargin;
#elif defined(__APPLE__)
        pthread_t self = pthread_self();
        const auto top = reinterpret_cast<std::uintptr_t>(pthread_get_stackaddr_np(self));
        return top - pthread_get_stacksize_np(self) + kStackMargin;
#elif defined(__linux__)
        pthread_attr_t attributes;
        if (pthread_getattr_np(pthread_self(), &attributes) != 0) return std::nullopt;
        void* low = nullptr;
        size_t size = 0;
        const int status = pthread_attr_getstack(&attributes, &low, &size);
        pthread_attr_destroy(&attributes);
        if (status != 0) return std::nullopt;
        return reinterpret_cast<std::uintptr_t>(low) + kStackMargin;
#else
        return std::nullopt;
#endif
    }
}
