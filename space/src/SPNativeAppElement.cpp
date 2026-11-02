#include "SPNativeAppElement.h"

#include <stdexcept>

#include "SPWindowElement.h"
#include "SPXmlParser.h"

namespace space {
    SPNativeAppElement::SPNativeAppElement(SPElement* parent, const SPAppBundle& bundle)
        : SPVBoxElement(parent)
        , m_bundle(bundle)
        , m_app(std::make_unique<SPNativeApp>(bundle)) {
    }

    void SPNativeAppElement::restart() {
        m_app = std::make_unique<SPNativeApp>(m_bundle);
    }

    void SPNativeAppElement::request(const string& method, const string& uri, const string& data) {
        nlohmann::json requestData = nlohmann::json::object();
        if (!data.empty()) {
            try {
                nlohmann::json parsed = nlohmann::json::parse(data);
                if (parsed.is_object())
                    requestData = std::move(parsed);
            } catch (const std::exception&) {
            }
        }

        SPNativeApp::Response response;
        SPXmlNode root;
        try {
            response = m_app->handle(method, uri, requestData);
            std::string parseError;
            if (response.redirect.empty() && !parseSPXml(response.xul, root, &parseError))
                throw std::runtime_error("The view for " + uri + " is not valid XUL: " + parseError);
        } catch (const std::exception& error) {
            reportError(error.what());
            return;
        }

        if (response.redirect.empty()) {
            loadXULFromXMLDoc(root);
            return;
        }
        // A redirect back into this app re-enters request() before navigate()
        // returns, so the depth stops redirect loops.
        if (m_redirects >= kMaxRedirects) {
            reportError("Too many redirects, the last from " + uri + " to " + response.redirect);
            return;
        }
        ++m_redirects;
        getWindowElement()->navigate(response.redirect);
        --m_redirects;
    }

    void SPNativeAppElement::reportError(const std::string& message) {
        if (m_errorHandler)
            m_errorHandler(message);
    }
}
