#include "swimming/V8Engine.h"

#include <cstddef>
#include <cstdint>
#include <mutex>

#include "StackLimit.h"

// The subset of rusty_v8's extern "C" binding layer (src/binding.cc, v152.2.0)
// this engine uses. V8 objects are opaque: a handle is the address of its
// slot, which the bindings take by reference (a pointer in the ABI) and turn
// back into a v8::Local; global handles are used the same way.
namespace swimming::v8raw {
    struct Platform;
    struct Value;
    struct Context;
    struct Object;
    struct String;
    struct Function;
    struct Script;
    struct External;
    struct CallbackInfo; // v8::FunctionCallbackInfo<v8::Value>

    using FunctionCallback = void (*)(const CallbackInfo* info);

    // v8::Maybe<double>
    struct MaybeDouble {
        bool hasValue;
        double value;
    };

    // v8::ResourceConstraints and v8::Isolate::CreateParams, as rusty_v8
    // mirrors them (src/isolate_create_params.rs); checked against
    // v8__Isolate__CreateParams__SIZEOF before use.
    struct ResourceConstraints {
        std::size_t codeRangeSize;
        std::size_t maxOldGenerationSize;
        std::size_t maxYoungGenerationSize;
        std::size_t initialOldGenerationSize;
        std::size_t initialYoungGenerationSize;
        std::uint64_t physicalMemorySize;
        std::uint32_t* stackLimit;
    };
    struct CreateParams {
        const void* codeEventHandler;
        ResourceConstraints constraints;
        const void* snapshotBlob;
        const void* counterLookupCallback;
        const void* createHistogramCallback;
        const void* addHistogramSampleCallback;
        Allocator* arrayBufferAllocator;
        void* arrayBufferAllocatorShared[2]; // std::shared_ptr
        const std::intptr_t* externalReferences;
        bool allowAtomicsWait;
        const void* fatalErrorHandler;
        const void* oomErrorHandler;
        const void* cppHeap;
    };

    // Sizes of the stack-allocated scope objects (binding.cc's static_asserts
    // and the generated binding's v8__ScriptOrigin_SIZE).
    constexpr std::size_t kHandleScopeSize = 3 * sizeof(void*);
    constexpr std::size_t kTryCatchSize = 6 * sizeof(void*);
    constexpr std::size_t kScriptOriginSize = 40;

    constexpr int kMicrotasksExplicit = 0;  // v8::MicrotasksPolicy::kExplicit
    constexpr int kNewStringNormal = 0;     // v8::NewStringType::kNormal
    constexpr int kConstructorAllow = 1;    // v8::ConstructorBehavior::kAllow
    constexpr int kHasSideEffect = 0;       // v8::SideEffectType::kHasSideEffect
    constexpr int kReplaceInvalidUtf8 = 2;  // v8::String::WriteFlags::kReplaceInvalidUtf8
    constexpr int kMaybeNothing = 2;        // rusty_v8's MaybeBool::Nothing

    extern "C" {
        Platform* v8__Platform__NewDefaultPlatform(int threadPoolSize, bool idleTaskSupport);
        void v8__V8__SetFlagsFromString(const char* flags, std::size_t length);
        void v8__V8__InitializePlatform(Platform* platform);
        void v8__V8__Initialize();
        const char* v8__V8__GetVersion();

        Allocator* v8__ArrayBuffer__Allocator__NewDefaultAllocator();
        void v8__ArrayBuffer__Allocator__DELETE(Allocator* self);

        void v8__Isolate__CreateParams__CONSTRUCT(CreateParams* buf);
        std::size_t v8__Isolate__CreateParams__SIZEOF();
        Isolate* v8__Isolate__New(const CreateParams* params);
        void v8__Isolate__Dispose(Isolate* isolate);
        void v8__Isolate__Enter(Isolate* isolate);
        void v8__Isolate__Exit(Isolate* isolate);
        void v8__Isolate__SetMicrotasksPolicy(Isolate* isolate, int policy);
        void v8__Isolate__PerformMicrotaskCheckpoint(Isolate* isolate);
        const Context* v8__Isolate__GetCurrentContext(Isolate* isolate);
        const Value* v8__Isolate__ThrowException(Isolate* isolate, const Value* exception);

        void v8__HandleScope__CONSTRUCT(void* buf, Isolate* isolate);
        void v8__HandleScope__DESTRUCT(void* self);
        const Data* v8__Global__New(Isolate* isolate, const Data* other);
        void v8__Global__Reset(const Data* data);

        const Context* v8__Context__New(Isolate* isolate, const void* globalTemplate, const Value* globalObject,
                                        void* microtaskQueue);
        void v8__Context__Enter(const Context* self);
        void v8__Context__Exit(const Context* self);
        const Object* v8__Context__Global(const Context* self);

        const Value* v8__Undefined(Isolate* isolate);
        const Value* v8__Boolean__New(Isolate* isolate, bool value);
        const Value* v8__Number__New(Isolate* isolate, double value);
        double v8__Number__Value(const Value* self);
        const String* v8__String__NewFromUtf8(Isolate* isolate, const char* data, int newType, int length);
        int v8__String__Utf8Length(const String* self, Isolate* isolate);
        std::size_t v8__String__WriteUtf8_v2(const String* self, Isolate* isolate, char* buffer, std::size_t capacity,
                                             int flags, std::size_t* processedCharacters);

        bool v8__Value__IsBoolean(const Value* self);
        bool v8__Value__IsNumber(const Value* self);
        bool v8__Value__IsString(const Value* self);
        bool v8__Value__IsFunction(const Value* self);
        bool v8__Value__BooleanValue(const Value* self, Isolate* isolate);
        const String* v8__Value__ToString(const Value* self, const Context* context);

        const Object* v8__Object__New(Isolate* isolate);
        int v8__Object__Set(const Object* self, const Context* context, const Value* key, const Value* value);
        const Value* v8__Object__Get(const Object* self, const Context* context, const Value* key);

        const External* v8__External__New(Isolate* isolate, void* value);
        void* v8__External__Value(const External* self);
        const Value* v8__Exception__Error(const String* message);

        const Function* v8__Function__New(const Context* context, FunctionCallback callback, const Value* data,
                                          int length, int constructorBehavior, int sideEffectType);
        const Value* v8__Function__Call(const Function* self, const Context* context, const Value* receiver, int argc,
                                        const Value* const argv[]);
        Isolate* v8__FunctionCallbackInfo__GetIsolate(const CallbackInfo* self);
        const Value* v8__FunctionCallbackInfo__Data(const CallbackInfo* self);
        const Value* v8__FunctionCallbackInfo__Get(const CallbackInfo* self, int index);
        int v8__FunctionCallbackInfo__Length(const CallbackInfo* self);
        std::uintptr_t* v8__FunctionCallbackInfo__GetReturnValue(const CallbackInfo* self);
        void v8__ReturnValue__Value__Set(std::uintptr_t** self, const Value* value); // v8::ReturnValue<v8::Value>*

        void v8__TryCatch__CONSTRUCT(void* buf, Isolate* isolate);
        void v8__TryCatch__DESTRUCT(void* self);
        bool v8__TryCatch__HasCaught(const void* self);
        const Value* v8__TryCatch__Exception(const void* self);
        const Value* v8__TryCatch__StackTrace(const void* self, const Context* context);

        void v8__ScriptOrigin__CONSTRUCT(void* buf, const Value* resourceName, int lineOffset, int columnOffset,
                                         bool sharedCrossOrigin, int scriptId, const Value* sourceMapUrl, bool opaque,
                                         bool isWasm, bool isModule, const Data* hostDefinedOptions);
        const Script* v8__Script__Compile(const Context* context, const String* source, const void* origin);
        const Value* v8__Script__Run(const Script* script, const Context* context);
    }
}

namespace swimming {
    using namespace v8raw;

    namespace {
        void initializeV8() {
            static std::once_flag once;
            std::call_once(once, [] {
                // Temporal is implemented by the temporal_rs Rust crate, which
                // is not linked (see V8RustStubs.cpp).
                const char flags[] = "--no-harmony-temporal";
                v8__V8__SetFlagsFromString(flags, sizeof(flags) - 1);
                // Never disposed: V8 cannot be initialized again in a process.
                v8__V8__InitializePlatform(v8__Platform__NewDefaultPlatform(0, false));
                v8__V8__Initialize();
            });
        }

        const String* newString(Isolate* isolate, const std::string& s) {
            const String* out = v8__String__NewFromUtf8(isolate, s.data(), kNewStringNormal, static_cast<int>(s.size()));
            if (!out) throw JsError("string is too long for V8");
            return out;
        }

        std::string toStdString(Isolate* isolate, const String* s) {
            const int length = v8__String__Utf8Length(s, isolate);
            std::string out(static_cast<std::size_t>(length > 0 ? length : 0), '\0');
            if (length > 0) v8__String__WriteUtf8_v2(s, isolate, out.data(), out.size(), kReplaceInvalidUtf8, nullptr);
            return out;
        }

        JsValue fromV8(Isolate* isolate, const Value* value) {
            if (!value) return {};
            if (v8__Value__IsBoolean(value)) return JsValue(v8__Value__BooleanValue(value, isolate));
            if (v8__Value__IsNumber(value)) return JsValue(v8__Number__Value(value));
            if (v8__Value__IsString(value)) return JsValue(toStdString(isolate, reinterpret_cast<const String*>(value)));
            return {};
        }

        const Value* toV8(Isolate* isolate, const JsValue& value) {
            if (value.isBool()) return v8__Boolean__New(isolate, value.isTrue());
            if (value.isNumber()) return v8__Number__New(isolate, value.asNumber());
            if (value.isString()) return reinterpret_cast<const Value*>(newString(isolate, value.asString()));
            return v8__Undefined(isolate);
        }

        /// A v8::TryCatch on the stack.
        class TryCatch {
        public:
            explicit TryCatch(Isolate* isolate) { v8__TryCatch__CONSTRUCT(m_storage, isolate); }
            ~TryCatch() { v8__TryCatch__DESTRUCT(m_storage); }
            TryCatch(const TryCatch&) = delete;
            TryCatch& operator=(const TryCatch&) = delete;

            /// The caught exception's stack trace (which includes its
            /// message) or else its string form.
            JsError error(Isolate* isolate, const Context* context) const {
                if (!v8__TryCatch__HasCaught(m_storage)) return JsError("execution terminated");
                const Value* trace = v8__TryCatch__StackTrace(m_storage, context);
                if (trace && v8__Value__IsString(trace))
                    return JsError(toStdString(isolate, reinterpret_cast<const String*>(trace)));
                const String* text = v8__Value__ToString(v8__TryCatch__Exception(m_storage), context);
                return JsError(text ? toStdString(isolate, text) : "(unprintable exception)");
            }

        private:
            alignas(void*) unsigned char m_storage[kTryCatchSize];
        };

        /// The callback of every native function; the NativeFunction is the
        /// function's data, wrapped in a v8::External.
        void trampoline(const CallbackInfo* info) {
            Isolate* isolate = v8__FunctionCallbackInfo__GetIsolate(info);
            const auto* external = reinterpret_cast<const External*>(v8__FunctionCallbackInfo__Data(info));
            const auto* native = static_cast<const NativeFunction*>(v8__External__Value(external));
            const int length = v8__FunctionCallbackInfo__Length(info);
            std::vector<JsValue> args;
            args.reserve(static_cast<std::size_t>(length));
            for (int i = 0; i < length; ++i)
                args.push_back(fromV8(isolate, v8__FunctionCallbackInfo__Get(info, i)));
            // C++ exceptions must not unwind through V8's frames.
            try {
                const JsValue result = (*native)(args);
                std::uintptr_t* returnValue = v8__FunctionCallbackInfo__GetReturnValue(info);
                v8__ReturnValue__Value__Set(&returnValue, toV8(isolate, result));
            } catch (const std::exception& error) {
                v8__Isolate__ThrowException(isolate, v8__Exception__Error(newString(isolate, error.what())));
            }
        }
    }

    /// Enters the isolate, a handle scope and the context for one call.
    class V8Engine::Scope {
    public:
        explicit Scope(const V8Engine& engine)
            : isolate(engine.m_isolate), context(reinterpret_cast<const Context*>(engine.m_context)) {
            v8__Isolate__Enter(isolate);
            v8__HandleScope__CONSTRUCT(m_handles, isolate);
            v8__Context__Enter(context);
        }
        ~Scope() {
            v8__Context__Exit(context);
            v8__HandleScope__DESTRUCT(m_handles);
            v8__Isolate__Exit(isolate);
        }
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

        Isolate* const isolate;
        const Context* const context;

    private:
        alignas(void*) unsigned char m_handles[kHandleScopeSize];
    };

    V8Engine::V8Engine() {
        initializeV8();
        if (v8__Isolate__CreateParams__SIZEOF() != sizeof(CreateParams))
            throw JsError("this V8 library's Isolate::CreateParams does not match swimming's");
        CreateParams params;
        v8__Isolate__CreateParams__CONSTRUCT(&params);
        m_allocator = v8__ArrayBuffer__Allocator__NewDefaultAllocator();
        params.arrayBufferAllocator = m_allocator;
        // V8 fixes its stack limit for the thread when the isolate is created;
        // its default allowance would run past a 1 MiB main thread stack.
        if (const auto limit = stackLimit())
            params.constraints.stackLimit = reinterpret_cast<std::uint32_t*>(*limit);
        m_isolate = v8__Isolate__New(&params);
        if (!m_isolate) {
            v8__ArrayBuffer__Allocator__DELETE(m_allocator);
            throw JsError("cannot create a V8 isolate");
        }
        // Match the other engines: promise jobs only run when the bridge drains them.
        v8__Isolate__SetMicrotasksPolicy(m_isolate, kMicrotasksExplicit);

        v8__Isolate__Enter(m_isolate);
        alignas(void*) unsigned char handles[kHandleScopeSize];
        v8__HandleScope__CONSTRUCT(handles, m_isolate);
        const Context* context = v8__Context__New(m_isolate, nullptr, nullptr, nullptr);
        m_context = context ? v8__Global__New(m_isolate, reinterpret_cast<const Data*>(context)) : nullptr;
        v8__HandleScope__DESTRUCT(handles);
        v8__Isolate__Exit(m_isolate);
        if (!m_context) {
            v8__Isolate__Dispose(m_isolate);
            v8__ArrayBuffer__Allocator__DELETE(m_allocator);
            throw JsError("cannot create a V8 context");
        }
    }

    V8Engine::~V8Engine() {
        v8__Global__Reset(m_context);
        v8__Isolate__Dispose(m_isolate);
        v8__ArrayBuffer__Allocator__DELETE(m_allocator);
    }

    const char* V8Engine::version() {
        initializeV8();
        return v8__V8__GetVersion();
    }

    void V8Engine::registerObject(const std::string& name,
                                  std::vector<std::pair<std::string, NativeFunction>> functions) {
        Scope scope(*this);
        const Object* object = v8__Object__New(scope.isolate);
        for (auto& [key, native] : functions) {
            m_natives.push_back(std::make_unique<NativeFunction>(std::move(native)));
            const auto* data = reinterpret_cast<const Value*>(v8__External__New(scope.isolate, m_natives.back().get()));
            const Function* function =
                v8__Function__New(scope.context, &trampoline, data, 0, kConstructorAllow, kHasSideEffect);
            if (!function) throw JsError("cannot create native function '" + key + "'");
            v8__Object__Set(object, scope.context, reinterpret_cast<const Value*>(newString(scope.isolate, key)),
                            reinterpret_cast<const Value*>(function));
        }
        const int set = v8__Object__Set(v8__Context__Global(scope.context), scope.context,
                                        reinterpret_cast<const Value*>(newString(scope.isolate, name)),
                                        reinterpret_cast<const Value*>(object));
        if (set == kMaybeNothing) throw JsError("cannot define '" + name + "'");
    }

    void V8Engine::eval(const std::string& source, const std::string& fileName) {
        Scope scope(*this);
        TryCatch tryCatch(scope.isolate);
        alignas(void*) unsigned char origin[kScriptOriginSize];
        v8__ScriptOrigin__CONSTRUCT(origin, reinterpret_cast<const Value*>(newString(scope.isolate, fileName)), 0, 0,
                                    false, -1, nullptr, false, false, false, nullptr);
        const Script* script = v8__Script__Compile(scope.context, newString(scope.isolate, source), origin);
        if (!script || !v8__Script__Run(script, scope.context))
            throw tryCatch.error(scope.isolate, scope.context);
    }

    std::optional<JsValue> V8Engine::callGlobal(const std::string& name, const std::vector<JsValue>& args) {
        Scope scope(*this);
        TryCatch tryCatch(scope.isolate);
        const Value* function = v8__Object__Get(v8__Context__Global(scope.context), scope.context,
                                                reinterpret_cast<const Value*>(newString(scope.isolate, name)));
        if (!function || !v8__Value__IsFunction(function)) return std::nullopt;
        std::vector<const Value*> argv;
        argv.reserve(args.size());
        for (const JsValue& arg : args)
            argv.push_back(toV8(scope.isolate, arg));
        const Value* result = v8__Function__Call(reinterpret_cast<const Function*>(function), scope.context,
                                                 v8__Undefined(scope.isolate), static_cast<int>(argv.size()), argv.data());
        if (!result) throw tryCatch.error(scope.isolate, scope.context);
        return fromV8(scope.isolate, result);
    }

    void V8Engine::runJobs() {
        v8__Isolate__Enter(m_isolate);
        v8__Isolate__PerformMicrotaskCheckpoint(m_isolate);
        v8__Isolate__Exit(m_isolate);
    }
}
