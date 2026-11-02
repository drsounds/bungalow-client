#ifndef SP_SWIMMING_APP_ELEMENT_H
#define SP_SWIMMING_APP_ELEMENT_H

#include <functional>
#include <string>

#include "SPAppBundle.h"
#include "SPSwimmingElement.h"

namespace space {
    /// Shows a Swimming app bundle (see SPSwimmingApp). bungalow:app:<app_id>:a:b
    /// maps to the in-app route /a/b, delivered through the polyfilled
    /// history and a popstate event, so routers that read window.location
    /// follow it.
    class SPSwimmingAppElement : public SPSwimmingElement {
    public:
        /// Starts the app; an app that fails to start reports why when navigated.
        SPSwimmingAppElement(SPElement* parent, const SPAppBundle& bundle);

        string getType() override { return "swimmingapp"; }

        /// Receives start and route errors.
        void setErrorHandler(std::function<void(const std::string&)> handler) {
            m_errorHandler = std::move(handler);
        }

        /// Starts the app again from its bundle, e.g. after it was rebuilt.
        void restart();

        /// Shows the in-app route path ("/a/b"), or reports why the app is
        /// not running. Returns false on error.
        bool navigate(const std::string& path);

    private:
        void reportError(const std::string& message);

        SPAppBundle m_bundle;
        std::string m_route;      // in-app path last delivered, e.g. "/a/b"
        std::string m_startError; // why the app is not running
        std::function<void(const std::string&)> m_errorHandler;
    };
}

#endif
