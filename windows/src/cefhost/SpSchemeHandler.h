#pragma once

#include <string>

#include "include/cef_scheme.h"

namespace space {
namespace cefhost {

// Serves sp://<bundle>/<path> from one bundle directory. Registered per app
// request context and per bundle host, so an app can only read its own files.
//
// Missing extensionless paths fall back to index.html so single-page apps can
// use client-side routes (sp://app/a/b -> index.html).
//
// allowAnyOrigin adds Access-Control-Allow-Origin: * to every response, for
// bundles that other apps load scripts from (the shared API bundle).
class SpSchemeHandlerFactory : public CefSchemeHandlerFactory {
public:
    explicit SpSchemeHandlerFactory(std::string bundleRoot, bool allowAnyOrigin = false);

    // Called on the IO thread.
    CefRefPtr<CefResourceHandler> Create(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                         const CefString& schemeName,
                                         CefRefPtr<CefRequest> request) override;

private:
    const std::string m_bundleRoot;
    const bool m_allowAnyOrigin;

    IMPLEMENT_REFCOUNTING(SpSchemeHandlerFactory);
};

} // namespace cefhost
} // namespace space
