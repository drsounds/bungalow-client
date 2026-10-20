#ifndef SP_NATIVE_APP_ELEMENT_H
#define SP_NATIVE_APP_ELEMENT_H

#include <functional>
#include <memory>
#include <string>

#include "SPNativeApp.h"
#include "SPVBoxElement.h"

namespace space {
    /// Shows a native (Lua MVC) app bundle. loadSrc(uri) renders the route
    /// for uri; buttons with an onclick in the rendered view post back to the
    /// same route with method POST, the form data and action = the onclick
    /// value. Controller redirects navigate the window.
    class SPNativeAppElement : public SPVBoxElement {
    public:
        /// Throws std::runtime_error if the app's routes are invalid.
        SPNativeAppElement(SPElement* parent, const SPAppBundle& bundle);

        string getType() override { return "nativeapp"; }

        /// Receives controller, view and template errors.
        void setErrorHandler(std::function<void(const std::string&)> handler) {
            m_errorHandler = std::move(handler);
        }

        /// Discards the app's Lua state and reloads its routes, so the next
        /// request runs freshly loaded controllers. Throws std::runtime_error
        /// if the routes are now invalid, keeping the running app.
        void restart();

        void request(const string& method, const string& uri, const string& data) override;

    private:
        static constexpr int kMaxRedirects = 8;

        void reportError(const std::string& message);

        SPAppBundle m_bundle;
        std::unique_ptr<SPNativeApp> m_app;
        std::function<void(const std::string&)> m_errorHandler;
        int m_redirects = 0; // redirects being followed (navigation is synchronous)
    };
}

#endif
