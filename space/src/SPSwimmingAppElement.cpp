#include "SPSwimmingAppElement.h"

#include <iostream>
#include <stdexcept>

#include "SPSwimmingApp.h"

namespace space {
    SPSwimmingAppElement::SPSwimmingAppElement(SPElement* parent, const SPAppBundle& bundle)
        : SPSwimmingElement(parent)
        , m_bundle(bundle) {
        setTagName("swimmingapp");
        restart();
    }

    void SPSwimmingAppElement::restart() {
        m_route = "/";
        m_startError.clear();
        std::string entry;
        try {
            entry = SPSwimmingApp::entryPath(m_bundle);
        } catch (const std::exception& error) {
            m_startError = error.what();
            stop();
            return;
        }
        try {
            start(entry);
        } catch (const swimming::JsError& error) {
            m_startError = "The app \"" + m_bundle.identifier + "\" failed to load: " + error.what();
        } catch (const std::exception& error) {
            m_startError = "The app \"" + m_bundle.identifier + "\" could not be started: " + error.what();
        }
    }

    bool SPSwimmingAppElement::navigate(const std::string& path) {
        invalidate();
        if (!getBridge()) {
            reportError(m_startError);
            return false;
        }
        if (m_route == path) return true;
        m_route = path;
        try {
            getBridge()->replaceRoute(path);
        } catch (const std::exception& error) {
            reportError(m_bundle.identifier + ": " + error.what());
            return false;
        }
        return true;
    }

    void SPSwimmingAppElement::reportError(const std::string& message) {
        if (m_errorHandler)
            m_errorHandler(message);
        else
            std::cerr << message << std::endl;
    }
}
