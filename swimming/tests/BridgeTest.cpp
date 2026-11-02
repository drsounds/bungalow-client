#include "swimming/Bridge.h"
#include "swimming/Loader.h"

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace swimming;
namespace fs = std::filesystem;

namespace {

int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << "\n";
        ++failures;
    }
}

bool throwsJsError(const std::function<void()>& f, const std::string& containing) {
    try {
        f();
    } catch (const JsError& error) {
        return std::string(error.what()).find(containing) != std::string::npos;
    }
    return false;
}

void texts(const LayoutBox& b, std::vector<std::string>& out) {
    if (b.text) out.push_back(*b.text);
    for (const auto& c : b.children) texts(c, out);
}

std::vector<std::string> texts(const Bridge& bridge) {
    std::vector<std::string> out;
    texts(bridge.engine().layout()->root, out);
    return out;
}

/// The same behavioural checks for every JS engine backend.
void conformance(std::unique_ptr<JsEngine> js) {
    const std::string name = js->name();
    Bridge b(200, 100, std::move(js));

    // Values round-trip through native functions.
    b.eval(R"(
        var N = __SWIMMING_NATIVE__;
        var root = N.createNode("div", "width:200px;height:100px", "{}");
        var text = N.createText("héllo");
        if (typeof root !== "number" || typeof text !== "number") throw new Error("ids must be numbers");
        N.appendChild(root, text);
        N.appendChild(0, root);
        N.commit();
    )");
    check(texts(b) == std::vector<std::string>{"h\xC3\xA9llo"}, name + ": strings round-trip as UTF-8");

    // Promise jobs are drained after eval.
    b.eval("var resolved = false; Promise.resolve().then(function () { resolved = true; });");
    b.eval("if (!resolved) throw new Error('microtasks were not run')");

    // Exceptions surface with their message.
    check(throwsJsError([&] { b.eval("throw new TypeError('boom')"); }, "boom"), name + ": exceptions carry their message");

    check(throwsJsError([&] { b.eval("undefinedFunction()"); }, "undefinedFunction"), name + ": reference errors surface");

    // Missing hooks are not an error.
    b.tick(16);

    // No engine has Temporal (V8's needs the temporal_rs Rust crate).
    b.eval("if (typeof Temporal !== 'undefined') throw new Error('Temporal is defined')");

    // Deep recursion is an exception, not a crash.
    check(throwsJsError([&] { b.eval("(function f() { f(); })()"); }, ""), name + ": stack overflow throws");
}

void jsBuildsTreeAndNativeEventsReachJs(const std::string& engine) {
    Bridge b(200, 100, createEngine(engine));
    b.eval(R"(
        const n = __SWIMMING_NATIVE__;
        globalThis.clicked = [];
        const box = n.createNode("div", "width:50px;height:20px;background-color:red", "{}");
        n.appendChild(0, box);
        n.commit();
        globalThis.__SWIMMING_DISPATCH__ = (id, ev) => clicked.push(id + ":" + ev);
    )");
    check(b.pointer("click", 5, 5) == std::optional<std::uint64_t>(1), "the click hits the box");
    check(!b.pointer("click", 150, 80), "a click beside it hits nothing");
    b.eval("if (clicked.join() !== '1:click') throw new Error('bad dispatch: ' + clicked)");
    check(b.render().pixel(2, 2) == Rgba{255, 0, 0, 255}, "the box is painted red");
}

void hostUrlResolvesLikeABrowser(const std::string& engine) {
    Bridge b(200, 100, createEngine(engine));
    b.eval(R"(
        [ new URL("/a/./b/../c?x=1#h", "http://swimming.local").href,
          new URL("d", "http://h:8/a/b").href,
          new URL("?q", "http://h/a#x").href,
          new URL("HTTP://h:8/p").port ]
          .forEach(function (s) { __SWIMMING_NATIVE__.appendChild(0, __SWIMMING_NATIVE__.createText(s)); });
        __SWIMMING_NATIVE__.commit();
        var doc = {}; document = doc;
        if (document.defaultView !== globalThis) throw new Error("document.defaultView is not the window");
    )");
    check(texts(b) == std::vector<std::string>{"http://swimming.local/a/c?x=1#h", "http://h:8/a/d", "http://h/a?q", "8"},
          "URL resolves paths, queries and ports");
}

void routesReachTheApp(const std::string& engine) {
    // Stands in for swimming-web-compat's history and swimming-react's
    // reconciler: shows location.pathname in a div.
    Bridge b(200, 100, createEngine(engine));
    b.eval(R"(
        var n = __SWIMMING_NATIVE__, path = "/", listeners = [];
        globalThis.history = { replaceState: function (s, t, url) { path = url; } };
        globalThis.dispatchEvent = function (e) { listeners.forEach(function (f) { f(e); }); };
        var div = n.createNode("div", "width: 100px; height: 40px; background-color: #ff0000", "{}");
        var text = n.createText(path);
        n.appendChild(div, text);
        n.appendChild(0, div);
        listeners.push(function () { n.setText(text, path); n.commit(); });
        n.commit();
    )");
    b.replaceRoute("/albums/1");
    check(texts(b) == std::vector<std::string>{"/albums/1"}, "the route reaches history and popstate listeners");
    b.replaceRoute("/a/\"b");
    check(texts(b) == std::vector<std::string>{"/a/\"b"}, "routes are escaped");
}

void timersRunWhenTicked(const std::string& engine) {
    Bridge b(200, 100, createEngine(engine));
    // A minimal __SWIMMING_TICK__ like swimming-web-compat's: one interval.
    b.eval(R"(
        var now = 0, fired = 0, at = 1000;
        globalThis.__SWIMMING_TICK__ = function (advance) {
          now += advance;
          if (now >= at) { at += 1000; fired++; }
          return false;
        };
    )");
    b.tick(1000);
    b.eval("if (fired !== 1) throw new Error('the interval did not fire: ' + fired)");
}

fs::path writeFiles(const std::string& name, const std::map<std::string, std::string>& files) {
    const fs::path dir = fs::temp_directory_path() / ("swimming-bridge-test-" + name);
    fs::remove_all(dir);
    for (const auto& [file, content] : files) {
        const fs::path path = dir / fs::u8path(file);
        fs::create_directories(path.parent_path());
        std::ofstream(path, std::ios::binary) << content;
    }
    return dir;
}

const char* const kPage =
    "<!doctype html><html><body><main style=\"width:120px;height:40px;background-color:red\"><p>hello</p></main>"
    "<script src=\"/app.js\"></script><script type=\"module\">throw new Error('modules are skipped')</script>"
    "<script>globalThis.inlineLoaded = true;</script></body></html>";

void checkPage(Bridge& bridge, const std::string& what) {
    bridge.eval("if (!inlineLoaded || externalLoaded !== 42) throw new Error('script tags were not run');");
    const LayoutBox& root = bridge.engine().layout()->root;
    check(root.children.size() == 1 && root.children[0].tag == "main", what + ": the body is mounted");
    check(!root.children.empty() && root.children[0].children[0].tag == "p", what + ": nested elements are mounted");
    check(!root.children.empty() && root.children[0].computed.background == Rgba{255, 0, 0, 255}, what + ": inline styles apply");
}

void loadsHtmlTreeAndExternalScriptFromHttp(const std::string& engine) {
    std::vector<std::string> requested;
    const Fetch fetch = [&](const std::string& url) -> std::string {
        requested.push_back(url);
        if (url == "http://127.0.0.1:8/app.js") return "globalThis.externalLoaded = 42;";
        if (url == "http://127.0.0.1:8/") return kPage;
        throw std::runtime_error("404 " + url);
    };
    Bridge bridge(200, 100, createEngine(engine));
    runSource(bridge, "http://127.0.0.1:8/", fetch);
    check(requested == std::vector<std::string>{"http://127.0.0.1:8/", "http://127.0.0.1:8/app.js"},
          "the page and its script are fetched");
    checkPage(bridge, "http");
}

void loadsHtmlTreeAndExternalScriptFromFile(const std::string& engine) {
    const fs::path dir = writeFiles("html", {{"index.html", kPage}, {"app.js", "globalThis.externalLoaded = 42;"}});
    Bridge bridge(200, 100, createEngine(engine));
    runSource(bridge, (dir / "index.html").u8string());
    checkPage(bridge, "file");
}

void resolvesUrlsLikeABrowser() {
    check(resolveUrl("http://h/a/b?q#f", "c") == "http://h/a/c", "relative path");
    check(resolveUrl("http://h/a/b", "../c/./d") == "http://h/c/d", "dot segments");
    check(resolveUrl("http://h/a/b", "/x?y") == "http://h/x?y", "absolute path");
    check(resolveUrl("https://h/a", "//cdn/x.js") == "https://cdn/x.js", "scheme-relative");
    check(resolveUrl("http://h", "x.js") == "http://h/x.js", "base without a path");
    check(resolveUrl("http://h/a?q", "#f") == "http://h/a?q#f", "fragment only");
    check(resolveUrl("http://h/a", "data:text/plain,x") == "data:text/plain,x", "absolute URL");
}

} // namespace

int main() {
    try {
        check(createEngine("quickjs") != nullptr && !createEngine("nope"), "engines are created by name");
        // The same behavioural checks for every engine built in.
        for (const std::string& engine : engineNames()) {
            auto js = createEngine(engine);
            std::cerr << "engine: " << engine << " (" << js->name() << ")\n";
            check(std::string(js->name()) == (engine == "v8" ? "V8" : "QuickJS"), "createEngine(\"" + engine + "\") is that engine");
            conformance(std::move(js));
            jsBuildsTreeAndNativeEventsReachJs(engine);
            hostUrlResolvesLikeABrowser(engine);
            routesReachTheApp(engine);
            timersRunWhenTicked(engine);
            loadsHtmlTreeAndExternalScriptFromHttp(engine);
            loadsHtmlTreeAndExternalScriptFromFile(engine);
        }
        resolvesUrlsLikeABrowser();
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << "\n";
        return 1;
    }
    return failures == 0 ? 0 : 1;
}
