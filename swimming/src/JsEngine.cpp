#include "swimming/JsEngine.h"

#include "swimming/QuickJsEngine.h"
#ifdef SWIMMING_HAS_V8
#include "swimming/V8Engine.h"
#endif

namespace swimming {
    std::unique_ptr<JsEngine> createDefaultEngine() {
#ifdef SWIMMING_DEFAULT_V8
        return createEngine("v8");
#else
        return createEngine("quickjs");
#endif
    }

    std::unique_ptr<JsEngine> createEngine(const std::string& name) {
        if (name == "quickjs") return std::make_unique<QuickJsEngine>();
#ifdef SWIMMING_HAS_V8
        if (name == "v8") return std::make_unique<V8Engine>();
#endif
        return nullptr;
    }

    std::vector<std::string> engineNames() {
#ifdef SWIMMING_HAS_V8
        return {"quickjs", "v8"};
#else
        return {"quickjs"};
#endif
    }
}
