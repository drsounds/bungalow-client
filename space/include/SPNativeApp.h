#ifndef SP_NATIVE_APP_H
#define SP_NATIVE_APP_H

#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "SPAppBundle.h"

struct lua_State;
struct lua_Debug;

namespace space {
    /// Runs an app bundle whose manifest declares "AppType": "Native": a Lua
    /// MVC app rendered with native XUL elements instead of an embedded browser.
    ///
    /// Routes come from <bundle>/routes.json, with paths relative to the
    /// bundle, and from the file each manifest Features.<name>.Routes names,
    /// with paths relative to that file's folder:
    ///
    ///     {"routes": [{"path": "^bungalow:app:x:(?P<name>\\w+)$",
    ///                  "view": "index.xml", "controller": "index.lua"}]}
    ///
    /// A view or controller without an extension gets .xml or .lua. The
    /// bundle's own routes come before feature routes, and the first route
    /// whose path matches the whole URI handles it. The controller's function
    /// named after its file (index.lua -> index) is called with
    /// request = {method, uri, action, data, arguments}, where
    /// arguments holds the captures from index 0 and by group name. The table
    /// it returns is the view's model; its fields are also template variables.
    ///
    /// The app keeps one Lua state for its lifetime and each controller file
    /// its own environment in it, so controllers keep state between requests.
    /// Controllers run sandboxed: the safe standard libraries, require() of
    /// .lua modules inside the bundle, and the libraries the manifest lists
    /// under ApiPermissions ("io", "os"). A request that runs more than the
    /// instruction limit fails, so a runaway controller cannot hang the app.
    ///
    /// Instead of a model, a controller can return
    ///   redirect(uri)          to navigate to uri (e.g. after handling a POST), or
    ///   render(view, model)    to render another view, named like a route's.
    class SPNativeApp {
    public:
        struct Response {
            std::string xul;      ///< The rendered view; empty for a redirect
            std::string redirect; ///< URI to navigate to instead, if not empty
        };

        static constexpr long long kDefaultInstructionLimit = 50'000'000;

        /// Throws std::runtime_error if a routes file is invalid.
        explicit SPNativeApp(const SPAppBundle& bundle);
        ~SPNativeApp();
        SPNativeApp(const SPNativeApp&) = delete;
        SPNativeApp& operator=(const SPNativeApp&) = delete;

        static bool isNative(const SPAppBundle& bundle);

        /// Runs the route matching uri. data is the posted form data; its
        /// "command" is the request action. Throws std::runtime_error if no
        /// route matches or the controller or view fails.
        Response handle(const std::string& method, const std::string& uri,
                        const nlohmann::json& data = nlohmann::json::object());

        /// Lua instructions one request may run (approximately).
        void setInstructionLimit(long long value) { m_instructionLimit = value; }

    private:
        struct Route {
            std::regex path;
            std::vector<std::string> groupNames; // per capture, empty if unnamed
            std::string base;                    // absolute paths: the routes file's folder,
            std::string view;                    // which render() views are relative to
            std::string controller;
        };
        struct LuaCloser {
            void operator()(lua_State* state) const;
        };

        static void countInstructions(lua_State* state, lua_Debug* debug);

        void loadRoutes(const std::string& file);
        void openSandbox(const nlohmann::json& manifest);
        void pushController(const std::string& path);
        void pushRequest(const Route& route, const std::smatch& match, const std::string& method,
                         const std::string& uri, const std::string& action, const nlohmann::json& data);

        std::string m_root;
        std::vector<Route> m_routes;
        std::unique_ptr<lua_State, LuaCloser> m_state;
        int m_sandbox = 0;                    // registry ref of the sandbox globals
        std::map<std::string, int> m_controllers; // path -> registry ref of its environment
        long long m_instructionLimit = kDefaultInstructionLimit;
        long long m_instructions = 0; // run by the current request
    };
}

#endif
