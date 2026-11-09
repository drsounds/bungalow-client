#include "service/BackgroundWorker.h"

namespace space {
namespace service {

BackgroundWorker::BackgroundWorker() : m_thread([this] { run(); }) {}

BackgroundWorker::~BackgroundWorker() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopping = true;
        m_tasks.clear();
    }
    m_wake.notify_all();
    if (m_thread.joinable()) m_thread.join();
}

void BackgroundWorker::post(std::function<void()> task) {
    if (!task) return;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stopping) return;
        m_tasks.push_back(std::move(task));
    }
    m_wake.notify_one();
}

void BackgroundWorker::run() {
    for (;;) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_wake.wait(lock, [this] { return m_stopping || !m_tasks.empty(); });
            if (m_stopping) return;
            task = std::move(m_tasks.front());
            m_tasks.pop_front();
        }
        try {
            task();
        } catch (...) {
            // A failing task must not take the worker down.
        }
    }
}

} // namespace service
} // namespace space
