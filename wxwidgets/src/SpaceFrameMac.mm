#import <AppKit/AppKit.h>

void ConfigureMacTitlebar(void* nsWindowPtr)
{
    NSWindow* window = (__bridge NSWindow*)nsWindowPtr;

    // Extend content view to fill the full window including the title bar area.
    // The traffic-light buttons remain visible and float above the content.
    window.styleMask             |= NSWindowStyleMaskFullSizeContentView;
    window.titlebarAppearsTransparent = YES;
    window.titleVisibility        = NSWindowTitleHidden;
}