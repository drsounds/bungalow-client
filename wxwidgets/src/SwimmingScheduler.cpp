#include "SwimmingScheduler.h"

#include <wx/timer.h>

#include "SPSwimmingElement.h"

namespace space {
namespace platform {
namespace wxwidgets {

namespace {
    class TickTimer : public wxTimer {
    public:
        void Notify() override { SPSwimmingElement::tickAll(); }
    };
}

void installSwimmingScheduler() {
    // Never destroyed: a wxTimer must not outlive wxWidgets' shutdown.
    static TickTimer* timer = new TickTimer();
    SPSwimmingElement::s_scheduleTick = [](int delayMs) { timer->StartOnce(delayMs); };
}

} // namespace wxwidgets
} // namespace platform
} // namespace space
