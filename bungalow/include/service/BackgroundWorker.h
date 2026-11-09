#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

namespace space {
namespace service {

/// Runs a task somewhere: later, and possibly on another thread.
using Executor = std::function<void(std::function<void()> task)>;

/// Runs each task at once, on the calling thread.
inline Executor inlineExecutor() {
    return [](std::function<void()> task) { if (task) task(); };
}

/// One thread that runs posted tasks in order. Service lookups go through a
/// single worker so a back-end never sees two of them at once.
class BackgroundWorker {
public:
    BackgroundWorker();
    /// Drops the tasks still queued and waits for the running one.
    ~BackgroundWorker();

    BackgroundWorker(const BackgroundWorker&) = delete;
    BackgroundWorker& operator=(const BackgroundWorker&) = delete;

    /// Queue @p task. Safe from any thread. An exception it throws is dropped.
    void post(std::function<void()> task);

    /// An Executor that posts here. It must not be used after the worker is destroyed.
    Executor executor() {
        return [this](std::function<void()> task) { post(std::move(task)); };
    }

private:
    void run();

    std::mutex                        m_mutex;
    std::condition_variable           m_wake;
    std::deque<std::function<void()>> m_tasks;
    bool                              m_stopping = false;
    std::thread                       m_thread; // last, so it starts after the rest
};

} // namespace service
} // namespace space
