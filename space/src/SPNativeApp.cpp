#include "SPNativeApp.h"

#include <lua.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>

#include "SPTemplate.h"

namespace fs = std::filesystem;

namespace space {
    namespace {
        constexpr int kMaxModelDepth = 32;
        // Instructions between instruction limit checks.
        constexpr int kHookInterval = 1000;
        // Metatable marking the tables redirect() and render() return.
        constexpr const char* kResultType = "bungalow.native.result";

        // Globals of the base library that cannot reach outside the sandbox
        // (no load, loadfile, dofile or collectgarbage).
        const char* const kBaseFunctions[] = {
            "assert", "error", "getmetatable", "ipairs", "next", "pairs", "pcall", "print",
            "rawequal", "rawget", "rawlen", "rawset", "select", "setmetatable", "tonumber",
            "tostring", "type", "xpcall", "_VERSION",
        };
        const char* const kSafeLibraries[] = { "coroutine", "math", "string", "table", "utf8" };
        // The parts of os available without the "os" permission.
        const char* const kSafeOsFunctions[] = { "clock", "date", "difftime", "time" };
        // Libraries an app can ask for with ApiPermissions.
        const char* const kPermissionLibraries[] = { "io", "os" };

        std::string stringField(const nlohmann::json& obj, const char* key) {
            if (!obj.is_object()) return {};
            auto it = obj.find(key);
            return (it != obj.end() && it->is_string()) ? it->get<std::string>() : std::string();
        }

        bool readFile(const fs::path& file, std::string& out) {
            std::ifstream in(file, std::ios::binary);
            if (!in) return false;
            std::ostringstream ss;
            ss << in.rdbuf();
            out = ss.str();
            return true;
        }

        // Resolves relative against base; false if the result escapes root.
        bool resolveInside(const fs::path& root, const fs::path& base, const std::string& relative, fs::path& out) {
            out = (base / fs::u8path(relative)).lexically_normal();
            const fs::path rel = out.lexically_relative(root);
            return !rel.empty() && *rel.begin() != "..";
        }

        // resolveInside, adding defaultExtension when relative has none
        // ("views/index" -> views/index.xml).
        bool resolveFile(const fs::path& root, const fs::path& base, const std::string& relative,
                         const char* defaultExtension, fs::path& out) {
            if (!resolveInside(root, base, relative, out)) return false;
            if (!out.has_extension()) out += defaultExtension;
            return true;
        }

        // A bundle file's path for messages and Lua chunk names.
        std::string relativeName(const fs::path& root, const fs::path& file) {
            return file.lexically_relative(root).generic_u8string();
        }

        // Translates Python-style named groups, (?P<name>...), to the plain
        // groups std::regex understands, recording each capture's name
        // (empty for unnamed captures).
        std::string translatePattern(const std::string& pattern, std::vector<std::string>& groupNames) {
            std::string out;
            bool inClass = false;
            for (size_t i = 0; i < pattern.size(); ++i) {
                const char c = pattern[i];
                if (c == '\\' && i + 1 < pattern.size()) {
                    out += c;
                    out += pattern[++i];
                    continue;
                }
                if (inClass) {
                    inClass = c != ']';
                } else if (c == '[') {
                    inClass = true;
                } else if (c == '(') {
                    if (pattern.compare(i + 1, 3, "?P<") == 0) {
                        const size_t close = pattern.find('>', i + 4);
                        if (close == std::string::npos)
                            throw std::runtime_error("unterminated group name");
                        groupNames.push_back(pattern.substr(i + 4, close - i - 4));
                        out += '(';
                        i = close;
                        continue;
                    }
                    // (?:...), (?=...) and (?!...) do not capture.
                    if (pattern.compare(i + 1, 1, "?") != 0)
                        groupNames.emplace_back();
                }
                out += c;
            }
            return out;
        }

        std::runtime_error popLuaError(lua_State* L) {
            const char* message = lua_tostring(L, -1);
            std::runtime_error error(message ? message : "Unknown Lua error");
            lua_pop(L, 1);
            return error;
        }

        // Restores the Lua stack when a request ends, including by an exception.
        struct StackGuard {
            lua_State* state;
            int top;
            ~StackGuard() { lua_settop(state, top); }
        };

        // Pushes a new globals table that falls back to the sandbox.
        void pushEnvironment(lua_State* L, int sandboxIndex) {
            sandboxIndex = lua_absindex(L, sandboxIndex);
            lua_newtable(L);
            lua_createtable(L, 0, 1);
            lua_pushvalue(L, sandboxIndex);
            lua_setfield(L, -2, "__index");
            lua_setmetatable(L, -2);
        }

        // Pushes the Lua file as a function, or an error message. Returns a
        // Lua status code.
        int loadFile(lua_State* L, const fs::path& root, const fs::path& file) {
            std::string source;
            const std::string chunkName = "@" + relativeName(root, file);
            if (!readFile(file, source)) {
                lua_pushfstring(L, "cannot read %s", chunkName.c_str() + 1);
                return LUA_ERRFILE;
            }
            return luaL_loadbufferx(L, source.data(), source.size(), chunkName.c_str(), "t");
        }

        // require(name) for the sandbox: loads <bundle>/<name with dots as
        // slashes>.lua once, in its own environment.
        // Upvalues: bundle root, loaded modules, sandbox.
        int sandboxRequire(lua_State* L) {
            const char* name = luaL_checkstring(L, 1);
            lua_getfield(L, lua_upvalueindex(2), name);
            if (!lua_isnil(L, -1)) return 1;
            lua_pop(L, 1);

            // C++ objects stay in this scope: lua_error does not unwind them.
            bool inside = false;
            int status = LUA_OK;
            {
                std::string relative(name);
                std::replace(relative.begin(), relative.end(), '.', '/');
                const fs::path root = fs::u8path(lua_tostring(L, lua_upvalueindex(1)));
                fs::path file;
                inside = resolveInside(root, root, relative + ".lua", file);
                if (inside) status = loadFile(L, root, file);
            }
            if (!inside) return luaL_error(L, "module '%s' is outside the app", name);
            if (status != LUA_OK) return lua_error(L);

            pushEnvironment(L, lua_upvalueindex(3));
            lua_setupvalue(L, -2, 1); // the chunk's _ENV
            lua_pushstring(L, name);
            lua_call(L, 1, 1);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                lua_pushboolean(L, 1);
            }
            lua_pushvalue(L, -1);
            lua_setfield(L, lua_upvalueindex(2), name);
            return 1;
        }

        // redirect(uri): a controller result that navigates to uri.
        int luaRedirect(lua_State* L) {
            luaL_checkstring(L, 1);
            lua_createtable(L, 0, 2);
            lua_pushliteral(L, "redirect");
            lua_setfield(L, -2, "kind");
            lua_pushvalue(L, 1);
            lua_setfield(L, -2, "uri");
            luaL_setmetatable(L, kResultType);
            return 1;
        }

        // render(view, model): a controller result that renders another view.
        int luaRender(lua_State* L) {
            luaL_checkstring(L, 1);
            if (!lua_isnoneornil(L, 2))
                luaL_checktype(L, 2, LUA_TTABLE);
            lua_createtable(L, 0, 3);
            lua_pushliteral(L, "render");
            lua_setfield(L, -2, "kind");
            lua_pushvalue(L, 1);
            lua_setfield(L, -2, "view");
            lua_pushvalue(L, 2);
            lua_setfield(L, -2, "model");
            luaL_setmetatable(L, kResultType);
            return 1;
        }

        bool isResult(lua_State* L, int index) {
            if (!lua_getmetatable(L, index)) return false;
            luaL_getmetatable(L, kResultType);
            const bool result = lua_rawequal(L, -1, -2) != 0;
            lua_pop(L, 2);
            return result;
        }

        std::string luaStringField(lua_State* L, int index, const char* key) {
            lua_getfield(L, index, key);
            size_t size = 0;
            const char* text = lua_tolstring(L, -1, &size);
            std::string value = text ? std::string(text, size) : std::string();
            lua_pop(L, 1);
            return value;
        }

        // Converts a controller result to JSON. Tables with exactly the keys
        // 1..n become arrays; functions and other non-data values become null.
        nlohmann::json toJson(lua_State* L, int index, int depth) {
            index = lua_absindex(L, index);
            switch (lua_type(L, index)) {
            case LUA_TBOOLEAN:
                return lua_toboolean(L, index) != 0;
            case LUA_TNUMBER:
                if (lua_isinteger(L, index))
                    return static_cast<std::int64_t>(lua_tointeger(L, index));
                return lua_tonumber(L, index);
            case LUA_TSTRING: {
                size_t size = 0;
                const char* text = lua_tolstring(L, index, &size);
                return std::string(text, size);
            }
            case LUA_TTABLE:
                break;
            default:
                return nullptr;
            }
            if (depth >= kMaxModelDepth)
                throw std::runtime_error("The controller result is nested too deeply");

            nlohmann::json object = nlohmann::json::object();
            lua_Integer sequenceKeys = 0;
            lua_Integer maxKey = 0;
            lua_pushnil(L);
            while (lua_next(L, index) != 0) {
                std::string key;
                if (lua_type(L, -2) == LUA_TNUMBER && lua_isinteger(L, -2)) {
                    const lua_Integer n = lua_tointeger(L, -2);
                    if (n >= 1) {
                        ++sequenceKeys;
                        maxKey = std::max(maxKey, n);
                    }
                    key = std::to_string(n);
                } else if (lua_type(L, -2) == LUA_TSTRING) {
                    size_t size = 0;
                    const char* text = lua_tolstring(L, -2, &size);
                    key.assign(text, size);
                } else {
                    lua_pop(L, 1);
                    continue;
                }
                object[key] = toJson(L, -1, depth + 1);
                lua_pop(L, 1);
            }

            if (sequenceKeys == 0 || static_cast<size_t>(sequenceKeys) != object.size() || maxKey != sequenceKeys)
                return object;
            nlohmann::json array = nlohmann::json::array();
            for (lua_Integer i = 1; i <= maxKey; ++i)
                array.push_back(std::move(object[std::to_string(i)]));
            return array;
        }
    }

    void SPNativeApp::LuaCloser::operator()(lua_State* state) const {
        lua_close(state);
    }

    SPNativeApp::SPNativeApp(const SPAppBundle& bundle)
        : m_root(fs::u8path(bundle.path).lexically_normal().u8string())
        , m_state(luaL_newstate()) {
        if (!m_state)
            throw std::runtime_error("Unable to create Lua state");
        openSandbox(bundle.manifest);

        const fs::path root = fs::u8path(m_root);
        std::error_code ec;
        if (fs::is_regular_file(root / "routes.json", ec))
            loadRoutes((root / "routes.json").u8string());

        auto features = bundle.manifest.find("Features");
        if (features == bundle.manifest.end() || !features->is_object())
            return;
        for (auto& [name, feature] : features->items()) {
            const std::string routes = stringField(feature, "Routes");
            fs::path file;
            if (routes.empty() || !resolveInside(root, root, routes, file))
                throw std::runtime_error("Feature \"" + name + "\" needs a Routes file inside the app");
            loadRoutes(file.u8string());
        }
    }

    SPNativeApp::~SPNativeApp() = default;

    bool SPNativeApp::isNative(const SPAppBundle& bundle) {
        return stringField(bundle.manifest, "AppType") == "Native";
    }

    void SPNativeApp::loadRoutes(const std::string& file) {
        const fs::path root = fs::u8path(m_root);
        const fs::path path = fs::u8path(file);
        const std::string name = relativeName(root, path);

        std::string text;
        if (!readFile(path, text))
            throw std::runtime_error("Cannot read " + name);
        // An empty routes file declares no routes.
        if (text.find_first_not_of(" \t\r\n") == std::string::npos)
            return;

        nlohmann::json json;
        try {
            json = nlohmann::json::parse(text);
        } catch (const std::exception& e) {
            throw std::runtime_error(name + " is not valid JSON: " + e.what());
        }
        auto routes = json.is_object() ? json.find("routes") : json.end();
        if (routes == json.end() || !routes->is_array())
            throw std::runtime_error(name + " has no \"routes\" array");

        const fs::path base = path.parent_path();
        for (const auto& entry : *routes) {
            const std::string pattern = stringField(entry, "path");
            const std::string view = stringField(entry, "view");
            const std::string controller = stringField(entry, "controller");
            if (pattern.empty() || view.empty() || controller.empty())
                throw std::runtime_error(name + ": every route needs a path, view and controller");

            Route route;
            fs::path viewPath, controllerPath;
            if (!resolveFile(root, base, view, ".xml", viewPath) ||
                !resolveFile(root, base, controller, ".lua", controllerPath))
                throw std::runtime_error(name + ": route " + pattern + " refers to a file outside the app");
            route.base = base.u8string();
            route.view = viewPath.u8string();
            route.controller = controllerPath.u8string();
            try {
                route.path = std::regex(translatePattern(pattern, route.groupNames));
            } catch (const std::exception& e) {
                throw std::runtime_error(name + ": invalid route path " + pattern + ": " + e.what());
            }
            m_routes.push_back(std::move(route));
        }
    }

    void SPNativeApp::openSandbox(const nlohmann::json& manifest) {
        lua_State* L = m_state.get();
        luaL_openlibs(L);

        std::set<std::string> permissions;
        auto requested = manifest.find("ApiPermissions");
        if (requested != manifest.end() && requested->is_array()) {
            for (const auto& entry : *requested) {
                const std::string permission = entry.is_string() ? entry.get<std::string>() : entry.dump();
                if (std::find(std::begin(kPermissionLibraries), std::end(kPermissionLibraries), permission)
                        != std::end(kPermissionLibraries))
                    permissions.insert(permission);
                else
                    std::cerr << "Ignoring unknown ApiPermission " << permission << " in " << m_root << std::endl;
            }
        }

        lua_newtable(L);
        const int sandbox = lua_gettop(L);
        for (const char* name : kBaseFunctions) {
            lua_getglobal(L, name);
            lua_setfield(L, sandbox, name);
        }
        for (const char* name : kSafeLibraries) {
            lua_getglobal(L, name);
            lua_setfield(L, sandbox, name);
        }
        for (const std::string& name : permissions) {
            lua_getglobal(L, name.c_str());
            lua_setfield(L, sandbox, name.c_str());
        }
        if (!permissions.count("os")) {
            lua_getglobal(L, "os");
            lua_createtable(L, 0, static_cast<int>(std::size(kSafeOsFunctions)));
            for (const char* name : kSafeOsFunctions) {
                lua_getfield(L, -2, name);
                lua_setfield(L, -2, name);
            }
            lua_setfield(L, sandbox, "os");
            lua_pop(L, 1);
        }
        lua_pushvalue(L, sandbox);
        lua_setfield(L, sandbox, "_G");

        lua_pushstring(L, m_root.c_str());
        lua_newtable(L);
        lua_pushvalue(L, sandbox);
        lua_pushcclosure(L, sandboxRequire, 3);
        lua_setfield(L, sandbox, "require");

        luaL_newmetatable(L, kResultType);
        lua_pop(L, 1);
        lua_pushcfunction(L, luaRedirect);
        lua_setfield(L, sandbox, "redirect");
        lua_pushcfunction(L, luaRender);
        lua_setfield(L, sandbox, "render");

        m_sandbox = luaL_ref(L, LUA_REGISTRYINDEX);

        // Threads (coroutines) copy the extra space and hook from this one.
        *static_cast<SPNativeApp**>(lua_getextraspace(L)) = this;
        lua_sethook(L, countInstructions, LUA_MASKCOUNT, kHookInterval);
    }

    void SPNativeApp::countInstructions(lua_State* L, lua_Debug*) {
        SPNativeApp* app = *static_cast<SPNativeApp**>(lua_getextraspace(L));
        app->m_instructions += kHookInterval;
        if (app->m_instructions > app->m_instructionLimit)
            luaL_error(L, "the request exceeded the instruction limit");
    }

    // Pushes the controller's environment, running the file the first time.
    void SPNativeApp::pushController(const std::string& path) {
        lua_State* L = m_state.get();
        auto cached = m_controllers.find(path);
        if (cached != m_controllers.end()) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, cached->second);
            return;
        }

        if (loadFile(L, fs::u8path(m_root), fs::u8path(path)) != LUA_OK)
            throw popLuaError(L);
        lua_rawgeti(L, LUA_REGISTRYINDEX, m_sandbox);
        pushEnvironment(L, -1);
        lua_remove(L, -2);
        lua_pushvalue(L, -1);
        lua_setupvalue(L, -3, 1); // the chunk's _ENV
        lua_insert(L, -2);        // environment, chunk
        if (lua_pcall(L, 0, 0, 0) != LUA_OK)
            throw popLuaError(L);
        lua_pushvalue(L, -1);
        m_controllers[path] = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    void SPNativeApp::pushRequest(const Route& route, const std::smatch& match, const std::string& method,
                                  const std::string& uri, const std::string& action, const nlohmann::json& data) {
        lua_State* L = m_state.get();
        lua_createtable(L, 0, 5);
        lua_pushlstring(L, method.data(), method.size());
        lua_setfield(L, -2, "method");
        lua_pushlstring(L, uri.data(), uri.size());
        lua_setfield(L, -2, "uri");
        lua_pushlstring(L, action.data(), action.size());
        lua_setfield(L, -2, "action");
        SPTemplate::pushJson(L, data);
        lua_setfield(L, -2, "data");

        lua_newtable(L);
        for (size_t i = 1; i < match.size(); ++i) {
            if (!match[i].matched) continue;
            const std::string value = match[i].str();
            lua_pushlstring(L, value.data(), value.size());
            lua_rawseti(L, -2, static_cast<lua_Integer>(i - 1));
            if (i - 1 < route.groupNames.size() && !route.groupNames[i - 1].empty()) {
                lua_pushlstring(L, value.data(), value.size());
                lua_setfield(L, -2, route.groupNames[i - 1].c_str());
            }
        }
        lua_setfield(L, -2, "arguments");
    }

    SPNativeApp::Response SPNativeApp::handle(const std::string& method, const std::string& uri,
                                              const nlohmann::json& data) {
        const std::string action = stringField(data, "command");
        const fs::path root = fs::u8path(m_root);
        for (const Route& route : m_routes) {
            std::smatch match;
            if (!std::regex_match(uri, match, route.path))
                continue;

            Response response;
            fs::path viewPath = fs::u8path(route.view);
            nlohmann::json model = nlohmann::json::object();
            {
                lua_State* L = m_state.get();
                StackGuard guard{ L, lua_gettop(L) };
                m_instructions = 0;
                pushController(route.controller);
                const std::string function = fs::u8path(route.controller).stem().u8string();
                lua_getfield(L, -1, function.c_str());
                if (!lua_isfunction(L, -1))
                    throw std::runtime_error(relativeName(root, fs::u8path(route.controller)) +
                                             " does not define a function " + function + "()");
                pushRequest(route, match, method, uri, action, data);
                if (lua_pcall(L, 1, 1, 0) != LUA_OK)
                    throw popLuaError(L);

                if (lua_istable(L, -1) && isResult(L, -1)) {
                    if (luaStringField(L, -1, "kind") == "redirect") {
                        response.redirect = luaStringField(L, -1, "uri");
                        return response;
                    }
                    const std::string view = luaStringField(L, -1, "view");
                    if (!resolveFile(root, fs::u8path(route.base), view, ".xml", viewPath))
                        throw std::runtime_error(function + "() renders a view outside the app: " + view);
                    lua_getfield(L, -1, "model");
                }
                if (lua_istable(L, -1))
                    model = toJson(L, -1, 0);
                else if (!lua_isnil(L, -1))
                    throw std::runtime_error(function + "() must return a table, redirect(), render() or nil");
            }

            std::string view;
            if (!readFile(viewPath, view))
                throw std::runtime_error("Cannot read " + relativeName(root, viewPath));
            response.xul = SPTemplate::render(view, method, uri, action, data, model, true);
            return response;
        }
        throw std::runtime_error("No route matches " + uri);
    }
}
