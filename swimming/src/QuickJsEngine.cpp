#include "swimming/QuickJsEngine.h"

#include <quickjs.h>

namespace swimming {
    namespace {
        JsValue fromJs(JSContext* ctx, JSValueConst value) {
            if (JS_IsBool(value)) return JsValue(JS_ToBool(ctx, value) > 0);
            if (JS_IsNumber(value)) {
                double n = 0;
                JS_ToFloat64(ctx, &n, value);
                return JsValue(n);
            }
            if (JS_IsString(value)) {
                size_t length = 0;
                const char* s = JS_ToCStringLen(ctx, &length, value);
                if (!s) return {};
                std::string out(s, length);
                JS_FreeCString(ctx, s);
                return JsValue(std::move(out));
            }
            return {};
        }

        JSValue toJs(JSContext* ctx, const JsValue& value) {
            if (value.isBool()) return JS_NewBool(ctx, value.isTrue());
            if (value.isNumber()) return JS_NewFloat64(ctx, value.asNumber());
            if (value.isString()) return JS_NewStringLen(ctx, value.asString().data(), value.asString().size());
            return JS_UNDEFINED;
        }

        std::string toString(JSContext* ctx, JSValueConst value) {
            const char* s = JS_ToCString(ctx, value);
            if (!s) return "(unprintable exception)";
            std::string out(s);
            JS_FreeCString(ctx, s);
            return out;
        }

        /// Takes the pending exception, preferring "<message>\n<stack>" over its bare string.
        JsError pendingError(JSContext* ctx) {
            JSValue exception = JS_GetException(ctx);
            std::string message = toString(ctx, exception);
            if (JS_IsError(exception)) {
                JSValue stack = JS_GetPropertyStr(ctx, exception, "stack");
                if (JS_IsString(stack)) message += "\n" + toString(ctx, stack);
                JS_FreeValue(ctx, stack);
            }
            JS_FreeValue(ctx, exception);
            return JsError(message);
        }

        JSValue trampoline(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv, int magic, JSValueConst*) {
            auto* engine = static_cast<QuickJsEngine*>(JS_GetContextOpaque(ctx));
            std::vector<JsValue> args;
            args.reserve(static_cast<size_t>(argc));
            for (int i = 0; i < argc; ++i)
                args.push_back(fromJs(ctx, argv[i]));
            try {
                return toJs(ctx, engine->callNative(magic, args));
            } catch (const std::exception& error) {
                return JS_ThrowInternalError(ctx, "%s", error.what());
            }
        }
    }

    QuickJsEngine::QuickJsEngine() {
        m_runtime = JS_NewRuntime();
        m_context = m_runtime ? JS_NewContext(m_runtime) : nullptr;
        if (!m_context) {
            if (m_runtime) JS_FreeRuntime(m_runtime);
            throw JsError("cannot create a QuickJS runtime");
        }
        JS_SetContextOpaque(m_context, this);
    }

    QuickJsEngine::~QuickJsEngine() {
        JS_FreeContext(m_context);
        JS_FreeRuntime(m_runtime);
    }

    void QuickJsEngine::registerObject(const std::string& name,
                                       std::vector<std::pair<std::string, NativeFunction>> functions) {
        JS_UpdateStackTop(m_runtime);
        JSValue object = JS_NewObject(m_context);
        for (auto& [key, native] : functions) {
            const int index = static_cast<int>(m_natives.size());
            m_natives.push_back(std::make_unique<NativeFunction>(std::move(native)));
            JSValue function = JS_NewCFunctionData(m_context, &trampoline, 0, index, 0, nullptr);
            JS_SetPropertyStr(m_context, object, key.c_str(), function); // takes ownership
        }
        JSValue global = JS_GetGlobalObject(m_context);
        const int set = JS_SetPropertyStr(m_context, global, name.c_str(), object);
        JS_FreeValue(m_context, global);
        if (set < 0) throw pendingError(m_context);
    }

    void QuickJsEngine::eval(const std::string& source, const std::string& fileName) {
        JS_UpdateStackTop(m_runtime);
        JSValue result = JS_Eval(m_context, source.c_str(), source.size(), fileName.c_str(), JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(result)) throw pendingError(m_context);
        JS_FreeValue(m_context, result);
    }

    std::optional<JsValue> QuickJsEngine::callGlobal(const std::string& name, const std::vector<JsValue>& args) {
        JS_UpdateStackTop(m_runtime);
        JSValue global = JS_GetGlobalObject(m_context);
        JSValue function = JS_GetPropertyStr(m_context, global, name.c_str());
        JS_FreeValue(m_context, global);
        if (!JS_IsFunction(m_context, function)) {
            JS_FreeValue(m_context, function);
            return std::nullopt;
        }
        std::vector<JSValue> values;
        values.reserve(args.size());
        for (const JsValue& arg : args)
            values.push_back(toJs(m_context, arg));
        JSValue result = JS_Call(m_context, function, JS_UNDEFINED, static_cast<int>(values.size()), values.data());
        for (JSValue& value : values)
            JS_FreeValue(m_context, value);
        JS_FreeValue(m_context, function);
        if (JS_IsException(result)) throw pendingError(m_context);
        JsValue out = fromJs(m_context, result);
        JS_FreeValue(m_context, result);
        return out;
    }

    void QuickJsEngine::runJobs() {
        JS_UpdateStackTop(m_runtime);
        JSContext* ctx = nullptr;
        int ran;
        while ((ran = JS_ExecutePendingJob(m_runtime, &ctx)) != 0) {
            // A job that throws is dropped, like an unhandled rejection in a browser.
            if (ran < 0 && ctx) JS_FreeValue(ctx, JS_GetException(ctx));
        }
    }

    std::unique_ptr<JsEngine> createDefaultEngine() {
        return std::make_unique<QuickJsEngine>();
    }
}
