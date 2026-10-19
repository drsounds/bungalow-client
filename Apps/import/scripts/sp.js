/**
 * Apps API bootstrap: defines getSpotifyApi() and the module loader behind
 * sp.require().
 *
 * Load it into every app page before the app's own scripts (the host injects
 * it, or a page can include <script src="sp://import/scripts/sp.js">).
 *
 * Modules are CommonJS style files evaluated with these free variables:
 *   exports, module, sp, require
 *
 * Module names:
 *   "sp://import/scripts/api/models"  absolute, ".js" is appended
 *   "sp://timemachine/js/jquery"      absolute, another bundle
 *   "script", "scripts/api/table"     relative to the current app's root
 *
 * Loading is synchronous (sync XHR), like the original API, so
 * `var models = sp.require(...)` returns the module directly.
 */
(function (global) {
    "use strict";

    if (typeof global.getSpotifyApi === "function") return;

    var cache = {}; // resolved url -> module

    // In the host, apps are served from sp://<app>/. In dev mode (a plain
    // web server with the bundles side by side: /import/, /timemachine/),
    // sp://<bundle>/x is fetched from <origin>/<bundle>/x instead.
    var hosted = global.location.protocol === "sp:";

    function appId() {
        if (hosted) return global.location.host;
        return global.location.pathname.split("/")[1] || "";
    }

    function appRoot() {
        return "sp://" + appId() + "/";
    }

    // "/year/1990/2000" for both sp://timemachine/year/1990/2000 and
    // http://localhost/timemachine/year/1990/2000 (dev mode).
    function route() {
        var path = global.location.pathname;
        if (!hosted) path = path.slice(appId().length + 1) || "/";
        return path.replace(/\/index\.html?$/i, "/");
    }

    function routeToUrl(path) {
        return (hosted ? "" : "/" + appId()) + "/" + String(path).replace(/^\/+/, "");
    }

    function toFetchUrl(url) {
        if (hosted || url.indexOf("sp://") !== 0) return url;
        return global.location.origin + "/" + url.slice("sp://".length);
    }

    function resolve(name) {
        var url = /^[a-z][a-z0-9+.-]*:\/\//i.test(name) ? name : appRoot() + name.replace(/^\.?\//, "");
        if (!/\.js$/i.test(url)) url += ".js";
        return url;
    }

    function readText(url) {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", toFetchUrl(url), false);
        xhr.send(null);
        // Custom schemes may report status 0 on success.
        if (xhr.status !== 200 && !(xhr.status === 0 && xhr.responseText)) {
            throw new Error("sp.require: could not load " + url + " (status " + xhr.status + ")");
        }
        return xhr.responseText;
    }

    function load(name) {
        var url = resolve(name);
        if (cache[url]) return cache[url].exports;

        // Cached before evaluation, so a require cycle gets the partial exports.
        var module = { id: url, uri: url, exports: {} };
        cache[url] = module;

        try {
            var source = readText(url);
            // sourceURL gives modules their own entry in the devtools.
            var factory = new Function("exports", "module", "sp", "require",
                source + "\n//# sourceURL=" + url);
            factory.call(global, module.exports, module, api, load);
        } catch (e) {
            delete cache[url];
            console.error("sp.require(\"" + name + "\") failed:", e);
            throw e;
        }
        return module.exports;
    }

    // ── String helpers the original API added to String.prototype ────────

    function define(name, fn) {
        if (!String.prototype[name])
            Object.defineProperty(String.prototype, name, { value: fn, configurable: true, writable: true });
    }

    function escapeHtml(s) {
        return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
                .replace(/"/g, "&quot;").replace(/'/g, "&#39;");
    }

    function safeDecode(s) {
        try { return decodeURIComponent(s.replace(/\+/g, " ")); } catch (e) { return s; }
    }

    define("decodeForText", function () { return safeDecode(String(this)); });
    define("decodeForHTML", function () { return escapeHtml(safeDecode(String(this))); });
    define("decodeForLink", function () { return encodeURIComponent(String(this)); });
    define("encodeToHTML", function () { return escapeHtml(String(this)); });

    // ── sp object ────────────────────────────────────────────────────────

    // Low level host services. Everything is mocked until the C++ bridge
    // exists; window.spacify (installed by the host) is used when present.
    var core = {
        apiVersion: 1,
        appId: appId(),
        language: (global.navigator && global.navigator.language || "en").split("-")[0],
        country: "SE",
        product: "Bungalow",
        hosted: hosted,
        getAppRoot: appRoot,
        getRoute: route,
        routeToUrl: routeToUrl,
        toFetchUrl: toFetchUrl,
        readFile: function (path) {
            return readText(/^[a-z][a-z0-9+.-]*:\/\//i.test(path) ? path : appRoot() + path.replace(/^\.?\//, ""));
        },
        // Opens a spotify:/spacify: URI in the host's view stack.
        navigate: function (uri) {
            if (global.spacify && typeof global.spacify.navigate === "function")
                global.spacify.navigate(uri);
            else
                console.log("[sp.core] navigate (no host):", uri);
        }
    };

    var api = {
        version: 1,
        core: core,
        require: load,
        resolve: resolve
    };

    global.getSpotifyApi = function (version) {
        if (version !== undefined && version !== 1)
            console.warn("getSpotifyApi(" + version + "): only version 1 is implemented");
        return api;
    };
    // Some apps call require() without the sp. prefix.
    if (typeof global.require !== "function") global.require = load;
})(window);
