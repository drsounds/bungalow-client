// SiteViewMac.mm — macOS/WKWebView platform helpers for SiteView
#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

// Attempt to locate the WKWebView in a view hierarchy.
// wxWebView on macOS uses WKWebView as its backing NSView; GetNativeBackend()
// returns the WKWebView directly as a void*.  We guard with isKindOfClass: in
// case that ever changes.
static WKWebView* toWKWebView(void* handle) {
    if (!handle) return nil;
    id obj = (__bridge id)handle;
    if ([obj isKindOfClass:[WKWebView class]])
        return (WKWebView*)obj;
    // Fallback: look one level deep in the subview tree
    if ([obj isKindOfClass:[NSView class]]) {
        for (NSView* sub in [(NSView*)obj subviews]) {
            if ([sub isKindOfClass:[WKWebView class]])
                return (WKWebView*)sub;
        }
    }
    return nil;
}

// Called from SiteView::platformConfigureWebView after webview creation.
// Enables/disables WKWebView developer extras so that right-click
// "Inspect Element" works when developer mode is on.
extern "C" void SiteView_macConfigureWebView(void* handle, bool developerMode) {
    WKWebView* wkView = toWKWebView(handle);
    if (!wkView) return;
    [wkView.configuration.preferences
        setValue:(developerMode ? @YES : @NO)
          forKey:@"developerExtrasEnabled"];
}

// Called from SiteView::platformApplyColorScheme to force the webview's
// prefers-color-scheme media feature to match the app's color scheme setting.
// scheme: 1 = dark, 2 = light, 0 = follow OS (nil appearance).
extern "C" void SiteView_macApplyColorScheme(void* handle, int scheme) {
    WKWebView* wkView = toWKWebView(handle);
    if (!wkView) return;
    if (scheme == 1)
        wkView.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    else if (scheme == 2)
        wkView.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
    else
        wkView.appearance = nil; // SP_COLOR_SCHEME_DEFAULT: follow OS preference
}

// Called from SiteView::showInspector (Develop → Show Inspector menu item).
// Enables developer extras then opens the Web Inspector using the private
// _showInspector: selector that has been stable since Safari 9.
extern "C" void SiteView_macShowInspector(void* handle) {
    WKWebView* wkView = toWKWebView(handle);
    if (!wkView) return;

    // Ensure extras are enabled first
    [wkView.configuration.preferences setValue:@YES forKey:@"developerExtrasEnabled"];

    SEL sel = NSSelectorFromString(@"_showInspector:");
    if ([wkView respondsToSelector:sel])
        [wkView performSelector:sel withObject:nil];
}
