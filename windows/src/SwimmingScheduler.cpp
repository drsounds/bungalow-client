#include "SwimmingScheduler.h"

#include <Windows.h>
#include <cstdio>

#include "SPSwimmingElement.h"

namespace space {
namespace win {

namespace {
    UINT_PTR tickTimer = 0;

    VOID CALLBACK tickTimerProc(HWND, UINT, UINT_PTR id, DWORD) {
        fprintf(stderr, "TRACE proc %llu (pending %llu)\n", (unsigned long long)id, (unsigned long long)tickTimer);
        KillTimer(nullptr, tickTimer);
        tickTimer = 0;
        SPSwimmingElement::tickAll();
    }
}

void installSwimmingScheduler() {
    SPSwimmingElement::s_scheduleTick = [](int delayMs) {
        // Passing the pending timer's id reschedules it rather than adding one.
        tickTimer = SetTimer(nullptr, tickTimer, static_cast<UINT>(delayMs), tickTimerProc);
        fprintf(stderr, "TRACE settimer %d -> %llu\n", delayMs, (unsigned long long)tickTimer);
    };
}

} // namespace win
} // namespace space
