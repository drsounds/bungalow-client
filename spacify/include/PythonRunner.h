#pragma once
#include <string>
#include <thread>
#include <atomic>

class PythonRunner {
public:
    PythonRunner();
    ~PythonRunner();

    void start(const std::string& appDir);
    void stop();
    bool isRunning() const { return m_running.load(); }

private:
    std::thread m_thread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_shouldStop;
    std::string m_appDir;  // Store app directory for shutdown signaling
};
