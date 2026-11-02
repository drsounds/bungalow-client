#include "swimming/Bridge.h"

#include <cmath>
#include <iostream>

#include <nlohmann/json.hpp>

namespace swimming {
    namespace {
        /// Browser globals the embedded engine lacks and swimming-web-compat
        /// expects the host to provide: a URL covering what its memory history
        /// reads (absolute URLs and paths resolved against a base), and
        /// document.defaultView on the stand-in document it installs, which
        /// React Router's createBrowserHistory reads.
        const char* const kHostGlobals = R"JS(
if (typeof document === "undefined") {
  let current;
  Object.defineProperty(globalThis, "document", {
    configurable: true,
    get: () => current,
    set: (value) => {
      if (value && typeof value === "object" && !("defaultView" in value)) value.defaultView = globalThis;
      current = value;
    },
  });
}
if (typeof URL === "undefined") {
  const ABSOLUTE = /^([a-z][a-z0-9+.-]*:)\/\/([^/?#]*)([^?#]*)(\?[^#]*)?(#.*)?$/i;
  const normalize = (path) => {
    const out = [];
    for (const part of path.split("/").slice(1)) {
      if (part === "..") out.pop();
      else if (part !== ".") out.push(part);
    }
    if (/\/\.{0,2}$/.test(path) && out[out.length - 1] !== "") out.push("");
    return "/" + out.join("/");
  };
  globalThis.URL = class URL {
    constructor(input, base) {
      input = String(input);
      let m = ABSOLUTE.exec(input);
      if (!m) {
        if (base === undefined) throw new TypeError("Invalid URL: " + input);
        const b = new URL(base);
        const path = /^[^?#]*/.exec(input)[0];
        const rest = input.slice(path.length);
        const search = /^\?[^#]*/.exec(rest);
        const hash = /#.*$/.exec(rest);
        m = [input, b.protocol, b.host,
          !path ? b.pathname : path[0] === "/" ? path : b.pathname.replace(/[^/]*$/, "") + path,
          search ? search[0] : path ? "" : b.search, hash ? hash[0] : ""];
      }
      this.protocol = m[1].toLowerCase();
      this.host = m[2];
      this.hostname = m[2].replace(/:\d*$/, "");
      this.port = (/:(\d*)$/.exec(m[2]) || ["", ""])[1];
      this.pathname = normalize(m[3] || "/");
      this.search = m[4] && m[4] !== "?" ? m[4] : "";
      this.hash = m[5] && m[5] !== "#" ? m[5] : "";
    }
    get origin() { return this.protocol + "//" + this.host; }
    get href() { return this.origin + this.pathname + this.search + this.hash; }
    toString() { return this.href; }
    toJSON() { return this.href; }
  };
}
)JS";

        const JsValue& arg(const std::vector<JsValue>& args, size_t i) {
            static const JsValue undefined;
            return i < args.size() ? args[i] : undefined;
        }

        std::uint64_t id(const std::vector<JsValue>& args, size_t i) {
            const double n = arg(args, i).asNumber();
            return std::isfinite(n) && n >= 0 ? static_cast<std::uint64_t>(n) : ~std::uint64_t(0);
        }
    }

    Bridge::Bridge(float width, float height, std::unique_ptr<JsEngine> js) : m_js(std::move(js)) {
        m_engine.computeLayout(width, height);
        registerGlobals();
        m_js->eval(kHostGlobals, "<swimming host globals>");
    }

    void Bridge::resize(float width, float height) {
        m_engine.computeLayout(width, height);
    }

    void Bridge::registerGlobals() {
        Engine& g = m_engine;
        const JsValue undefined;
        m_js->registerObject("__SWIMMING_NATIVE__", {
            {"createNode", [&g](const std::vector<JsValue>& a) {
                return JsValue(static_cast<double>(g.createNode(arg(a, 0).asString(), arg(a, 1).asString(), arg(a, 2).asString())));
            }},
            {"createText", [&g](const std::vector<JsValue>& a) {
                return JsValue(static_cast<double>(g.createText(arg(a, 0).asString())));
            }},
            {"setText", [&g, undefined](const std::vector<JsValue>& a) {
                g.setText(id(a, 0), arg(a, 1).asString());
                return undefined;
            }},
            {"appendChild", [&g, undefined](const std::vector<JsValue>& a) {
                g.appendChild(id(a, 0), id(a, 1));
                return undefined;
            }},
            {"insertBefore", [&g, undefined](const std::vector<JsValue>& a) {
                const double before = arg(a, 2).asNumber();
                g.insertBefore(id(a, 0), id(a, 1), before >= 0 ? std::optional<std::uint64_t>(id(a, 2)) : std::nullopt);
                return undefined;
            }},
            {"removeChild", [&g, undefined](const std::vector<JsValue>& a) {
                g.removeChild(id(a, 0), id(a, 1));
                return undefined;
            }},
            {"updateNode", [&g, undefined](const std::vector<JsValue>& a) {
                g.updateNode(id(a, 0), arg(a, 1).asString(), arg(a, 2).asString());
                return undefined;
            }},
            {"commit", [&g, undefined](const std::vector<JsValue>&) {
                g.computeLayout(g.width(), g.height());
                return undefined;
            }},
        });
    }

    void Bridge::eval(const std::string& source, const std::string& fileName) {
        m_js->eval(source, fileName);
        runUntilIdle();
    }

    void Bridge::runUntilIdle() {
        for (int i = 0; i < 10000; ++i) {
            std::cerr << "TRACE idle " << i << std::endl; m_js->runJobs(); std::cerr << "TRACE jobs done" << std::endl;
            const auto pending = m_js->callGlobal("__SWIMMING_TICK__", {JsValue(0.0)});
            if (!pending || !pending->isTrue()) break;
        }
        m_js->runJobs();
    }

    void Bridge::tick(std::uint32_t elapsedMs) {
        m_js->callGlobal("__SWIMMING_TICK__", {JsValue(static_cast<double>(elapsedMs))});
        runUntilIdle();
    }

    std::optional<std::uint64_t> Bridge::pointer(const std::string& event, float x, float y) {
        const auto target = m_engine.hitTest(x, y);
        if (target) {
            m_js->callGlobal("__SWIMMING_DISPATCH__", {JsValue(static_cast<double>(*target)), JsValue(event)});
            runUntilIdle();
        }
        return target;
    }

    void Bridge::replaceRoute(const std::string& path) {
        const std::string quoted = nlohmann::json(path).dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
        eval("if(typeof history!=='undefined'){history.replaceState(null,''," + quoted + ");"
             "if(typeof dispatchEvent==='function')dispatchEvent({type:'popstate',state:null});}",
             "<swimming route>");
    }

    Pixmap Bridge::render() const {
        const LaidOut* l = m_engine.layout();
        const unsigned w = static_cast<unsigned>(m_engine.width()), h = static_cast<unsigned>(m_engine.height());
        return l ? paint(*l, w, h) : Pixmap(w, h);
    }
}
