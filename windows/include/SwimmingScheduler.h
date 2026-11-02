#pragma once

namespace space {
namespace win {

// Runs the frames of swimming apps (SPSwimmingElement::tickAll) from a
// thread timer. Call once on the UI thread.
void installSwimmingScheduler();

} // namespace win
} // namespace space
