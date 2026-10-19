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
class SpSchemeHandlerFactory : public CefSchemeHandlerFactory {
public:
    explicit SpSchemeHandlerFactory(std::string bundleRoot);

    // Called on the IO thread.
    CefRefPtr<CefResourceHandler> Create(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                         const CefString& schemeName,
                                         CefRefPtr<CefRequest> request) override;

private:
    const std::string m_bundleRoot;

    IMPLEMENT_REFCOUNTING(SpSchemeHandlerFactory);
};

} // namespace cefhost
} // namespace space
