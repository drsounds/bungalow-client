#ifndef SWIMMING_QUICKJS_ENGINE_H
#define SWIMMING_QUICKJS_ENGINE_H

#include <memory>
#include <vector>

#include "swimming/JsEngine.h"

struct JSRuntime;
struct JSContext;

namespace swimming {
    /// QuickJS (quickjs-ng) backend.
    class QuickJsEngine final : public JsEngine {
    public:
        QuickJsEngine();
        ~QuickJsEngine() override;
        QuickJsEngine(const QuickJsEngine&) = delete;
        QuickJsEngine& operator=(const QuickJsEngine&) = delete;

        const char* name() const override { return "QuickJS"; }
        void registerObject(const std::string& name,
                            std::vector<std::pair<std::string, NativeFunction>> functions) override;
        void eval(const std::string& source, const std::string& fileName) override;
        std::optional<JsValue> callGlobal(const std::string& name, const std::vector<JsValue>& args) override;
        void runJobs() override;

        /// Calls native function `index`; used by the JS-facing trampoline.
        JsValue callNative(int index, const std::vector<JsValue>& args) { return (*m_natives[index])(args); }

    private:
        JSRuntime* m_runtime = nullptr;
        JSContext* m_context = nullptr;
        // Indexed by the JS functions' magic; stable since they are never removed.
        std::vector<std::unique_ptr<NativeFunction>> m_natives;
    };
}

#endif
