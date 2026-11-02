#ifndef SWIMMING_SCHEDULER_H
#define SWIMMING_SCHEDULER_H

namespace space {
namespace platform {
namespace wxwidgets {

// Runs the frames of swimming apps (SPSwimmingElement::tickAll) from a
// wxTimer. Call once from wxApp::OnInit.
void installSwimmingScheduler();

} // namespace wxwidgets
} // namespace platform
} // namespace space

#endif
