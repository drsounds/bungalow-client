#ifndef SWIMMING_V8_ENGINE_H
#define SWIMMING_V8_ENGINE_H

#include <memory>
#include <vector>

#include "swimming/JsEngine.h"

namespace swimming {
    namespace v8raw {
        struct Isolate;
        struct Allocator;
        struct Data;
    }

    /// V8 backend, over the extern "C" bindings of rusty_v8's prebuilt
    /// library (V8 15.2), like the swimming Rust crates. Built with
    /// SWIMMING_ENABLE_V8.
    ///
    /// Each call enters the isolate and its context and leaves them again,
    /// so several engines can run on one thread. Promise jobs only run in
    /// runJobs(), as with the other engines.
    class V8Engine final : public JsEngine {
    public:
        /// Throws JsError if the isolate cannot be created.
        V8Engine();
        ~V8Engine() override;
        V8Engine(const V8Engine&) = delete;
        V8Engine& operator=(const V8Engine&) = delete;

        const char* name() const override { return "V8"; }
        void registerObject(const std::string& name,
                            std::vector<std::pair<std::string, NativeFunction>> functions) override;
        void eval(const std::string& source, const std::string& fileName) override;
        std::optional<JsValue> callGlobal(const std::string& name, const std::vector<JsValue>& args) override;
        void runJobs() override;

        /// V8's version, e.g. "15.2.124.1-rusty".
        static const char* version();

    private:
        class Scope;

        v8raw::Isolate* m_isolate = nullptr;
        v8raw::Allocator* m_allocator = nullptr;
        const v8raw::Data* m_context = nullptr; // global handle
        // Referenced from JS functions through v8::External; never removed.
        std::vector<std::unique_ptr<NativeFunction>> m_natives;
    };
}

#endif
