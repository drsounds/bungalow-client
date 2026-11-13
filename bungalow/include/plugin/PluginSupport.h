#pragma once

// Header-only helpers for implementing and consuming the interfaces of
// BungalowPlugin.h: a smart pointer, a base class that supplies reference
// counting and QueryInterface, and string conversion. Used by the host and
// free for plugins to use; nothing here crosses the plugin boundary, so a
// plugin may be built with any compiler and standard library.

#include "plugin/BungalowPlugin.h"

#include <atomic>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace space {
namespace com {

// ── Interface traits: the IID of an interface and the one it derives from ────

template <class I> struct InterfaceTraits;

#define BUNGALOW_COM_INTERFACE(Interface, BaseInterface)                        \
    template <> struct InterfaceTraits<Interface> {                             \
        static constexpr const Guid& iid() { return IID_##Interface; }          \
        using Base = BaseInterface;                                             \
    };

BUNGALOW_COM_INTERFACE(IString, IUnknown)
BUNGALOW_COM_INTERFACE(IAspect, IUnknown)
BUNGALOW_COM_INTERFACE(ISession, IUnknown)
BUNGALOW_COM_INTERFACE(ILoginCallback, IUnknown)
BUNGALOW_COM_INTERFACE(IAuthAspect, IUnknown)
BUNGALOW_COM_INTERFACE(IService, IAspect)
BUNGALOW_COM_INTERFACE(IPlaybackListener, IUnknown)
BUNGALOW_COM_INTERFACE(IMediaService, IUnknown)
BUNGALOW_COM_INTERFACE(IMusicService, IUnknown)
BUNGALOW_COM_INTERFACE(IFeature, IAspect)
BUNGALOW_COM_INTERFACE(IConceptContext, IUnknown)
BUNGALOW_COM_INTERFACE(IConcept, IAspect)
BUNGALOW_COM_INTERFACE(IApplicationContext, IUnknown)
BUNGALOW_COM_INTERFACE(IApplication, IAspect)

template <class I> constexpr const Guid& iidOf() {
    if constexpr (std::is_same_v<I, IUnknown>) return IID_IUnknown;
    else return InterfaceTraits<I>::iid();
}

namespace detail {
/// `p` as the interface `iid` names, if I is or derives from it.
template <class I> void* castChain(I* p, const Guid& iid) {
    if constexpr (std::is_same_v<I, IUnknown>) {
        return iid == IID_IUnknown ? p : nullptr;
    } else {
        if (iid == InterfaceTraits<I>::iid()) return p;
        return castChain<typename InterfaceTraits<I>::Base>(p, iid);
    }
}
} // namespace detail

// ── ComPtr ───────────────────────────────────────────────────────────────────

/// Owns one reference to a COM object.
template <class T> class ComPtr {
public:
    ComPtr() = default;
    ComPtr(std::nullptr_t) {}
    /// Shares `p`: takes a reference of its own.
    explicit ComPtr(T* p) : m_p(p) { if (m_p) m_p->AddRef(); }
    ComPtr(const ComPtr& o) : m_p(o.m_p) { if (m_p) m_p->AddRef(); }
    ComPtr(ComPtr&& o) noexcept : m_p(o.m_p) { o.m_p = nullptr; }
    ~ComPtr() { reset(); }

    ComPtr& operator=(ComPtr o) noexcept { std::swap(m_p, o.m_p); return *this; }

    /// Takes over a reference the caller already holds (a `new`ed ComObject,
    /// an out parameter).
    static ComPtr adopt(T* p) { ComPtr r; r.m_p = p; return r; }

    void reset() { if (T* p = m_p) { m_p = nullptr; p->Release(); } }
    T* get() const { return m_p; }
    T* operator->() const { return m_p; }
    explicit operator bool() const { return m_p != nullptr; }
    /// For out parameters; releases what is held first.
    T** put() { reset(); return &m_p; }
    /// Gives up the reference without releasing it.
    T* detach() { T* p = m_p; m_p = nullptr; return p; }

    /// QueryInterface; empty when the object does not implement I.
    template <class I> ComPtr<I> as() const {
        ComPtr<I> out;
        if (m_p && failed(m_p->QueryInterface(iidOf<I>(), reinterpret_cast<void**>(out.put())))) out.reset();
        return out;
    }

private:
    T* m_p = nullptr;
};

// ── ComObject ────────────────────────────────────────────────────────────────

/// Base of a COM object implementing `Interfaces`: reference counting and a
/// QueryInterface that answers each of them, their bases and IUnknown.
///
///     class MyService : public ComObject<IService, IAuthAspect> { … };
///     ComPtr<IService> s = make<MyService>(args…).as<IService>();   // or ComPtr<MyService>
///
/// Objects start with one reference and delete themselves at zero, in the
/// module that created them.
template <class... Interfaces> class ComObject : public Interfaces... {
public:
    Result BUNGALOW_CALL QueryInterface(const Guid& iid, void** out) override {
        if (!out) return kPointer;
        void* found = nullptr;
        ((found = found ? found : detail::castChain<Interfaces>(static_cast<Interfaces*>(this), iid)), ...);
        *out = found;
        if (!found) return kNoInterface;
        m_refs.fetch_add(1, std::memory_order_relaxed);
        return kOk;
    }
    uint32_t BUNGALOW_CALL AddRef() override { return m_refs.fetch_add(1, std::memory_order_relaxed) + 1; }
    uint32_t BUNGALOW_CALL Release() override {
        const uint32_t left = m_refs.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (left == 0) delete this;
        return left;
    }

protected:
    ComObject() = default;
    virtual ~ComObject() = default;
    ComObject(const ComObject&) = delete;
    ComObject& operator=(const ComObject&) = delete;

private:
    std::atomic<uint32_t> m_refs{1};
};

/// Creates a ComObject-derived object, owning its first reference.
template <class T, class... Args> ComPtr<T> make(Args&&... args) {
    return ComPtr<T>::adopt(new T(std::forward<Args>(args)...));
}

// ── Strings ──────────────────────────────────────────────────────────────────

class StringObject final : public ComObject<IString> {
public:
    explicit StringObject(std::string_view text) : m_text(text) {}
    const char* BUNGALOW_CALL Data() override { return m_text.c_str(); }
    uint32_t    BUNGALOW_CALL Size() override { return static_cast<uint32_t>(m_text.size()); }

private:
    std::string m_text;
};

/// Fills a string out parameter: `return returnString(m_name, out);`
inline Result returnString(std::string_view text, IString** out) {
    if (!out) return kPointer;
    *out = new StringObject(text);
    return kOk;
}

inline std::string toStd(IString* s) {
    if (!s) return {};
    const char* data = s->Data();
    return data ? std::string(data, s->Size()) : std::string();
}

/// Calls a string getter and returns its value, or "" when it fails or
/// returns nothing: `getString([&](IString** o) { return svc->GetId(o); })`.
template <class Getter> std::string getString(Getter&& getter) {
    ComPtr<IString> s;
    if (failed(getter(s.put()))) return {};
    return toStd(s.get());
}

} // namespace com
} // namespace space
