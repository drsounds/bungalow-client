#pragma once

#include <functional>

namespace space {
namespace win {

// Creates the hidden message window that receives work posted from other threads.
// Must be called once on the UI thread.
void initUiDispatcher();

// Queues fn to run on the UI thread. Safe to call from any thread.
void postToUi(std::function<void()> fn);

} // namespace win
} // namespace space
