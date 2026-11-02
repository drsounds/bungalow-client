#include "SwimmingScheduler.h"

#include <Windows.h>

#include "SPSwimmingElement.h"

namespace space {
namespace win {

namespace {
    UINT_PTR tickTimer = 0;

    VOID CALLBACK tickTimerProc(HWND, UINT, UINT_PTR, DWORD) {
        KillTimer(nullptr, tickTimer);
        tickTimer = 0;
        SPSwimmingElement::tickAll();
    }
}

void installSwimmingScheduler() {
    SPSwimmingElement::s_scheduleTick = [](int delayMs) {
        // Passing the pending timer's id reschedules it rather than adding one.
        tickTimer = SetTimer(nullptr, tickTimer, static_cast<UINT>(delayMs), tickTimerProc);
    };
}

} // namespace win
} // namespace space
