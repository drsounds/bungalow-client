#ifndef SWIMMING_JS_ENGINE_H
#define SWIMMING_JS_ENGINE_H

#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

/// The engine-neutral JavaScript interface used by the bridge. Backends
/// (QuickJsEngine, and V8Engine when built with SWIMMING_ENABLE_V8)
/// implement JsEngine; createDefaultEngine() picks one.
namespace swimming {
    /// A value crossing the native/JS boundary. Anything else (objects,
    /// functions, ...) is seen as undefined on the native side.
    class JsValue {
    public:
        JsValue() = default;
        JsValue(bool value) : m_value(value) {}
        JsValue(double value) : m_value(value) {}
        JsValue(std::string value) : m_value(std::move(value)) {}
        JsValue(const char* value) : m_value(std::string(value)) {}

        bool isUndefined() const { return std::holds_alternative<std::monostate>(m_value); }
        bool isBool() const { return std::holds_alternative<bool>(m_value); }
        bool isNumber() const { return std::holds_alternative<double>(m_value); }
        bool isString() const { return std::holds_alternative<std::string>(m_value); }

        /// The number, or NaN for other values.
        double asNumber() const {
            const double* n = std::get_if<double>(&m_value);
            return n ? *n : std::numeric_limits<double>::quiet_NaN();
        }
        /// The string, or "" for other values.
        const std::string& asString() const {
            static const std::string empty;
            const std::string* s = std::get_if<std::string>(&m_value);
            return s ? *s : empty;
        }
        bool isTrue() const {
            const bool* b = std::get_if<bool>(&m_value);
            return b && *b;
        }

        bool operator==(const JsValue& other) const { return m_value == other.m_value; }

    private:
        std::variant<std::monostate, bool, double, std::string> m_value;
    };

    /// A native function exposed to JS; receives the call's arguments. If it
    /// throws, the exception is rethrown in JS as an InternalError.
    using NativeFunction = std::function<JsValue(const std::vector<JsValue>&)>;

    /// A JS exception or engine failure, carrying the engine's rendering of
    /// the error (with the stack when there is one).
    class JsError : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    /// What the bridge needs from a JavaScript engine. Engines have no event
    /// loop of their own: the bridge drives timers through __SWIMMING_TICK__
    /// and drains promise jobs with runJobs(). All methods throw JsError.
    class JsEngine {
    public:
        virtual ~JsEngine() = default;

        /// Human-readable engine name, e.g. for diagnostics.
        virtual const char* name() const = 0;

        /// Defines globalThis[name] as an object whose methods call the given native functions.
        virtual void registerObject(const std::string& name,
                                    std::vector<std::pair<std::string, NativeFunction>> functions) = 0;

        /// Evaluates a classic (non-module) script in the global scope.
        virtual void eval(const std::string& source, const std::string& fileName = "<script>") = 0;

        /// Calls globalThis[name](...args); returns nullopt when it is not a function.
        virtual std::optional<JsValue> callGlobal(const std::string& name, const std::vector<JsValue>& args) = 0;

        /// Runs all pending promise jobs (microtasks).
        virtual void runJobs() = 0;
    };

    /// The engine the bridge uses unless it is given one: QuickJS, or V8 when
    /// built with SWIMMING_DEFAULT_ENGINE=v8.
    std::unique_ptr<JsEngine> createDefaultEngine();

    /// Creates the engine named "quickjs" or "v8"; null if it is not built in.
    std::unique_ptr<JsEngine> createEngine(const std::string& name);

    /// Names of the engines built in, e.g. {"quickjs", "v8"}.
    std::vector<std::string> engineNames();
}

#endif
