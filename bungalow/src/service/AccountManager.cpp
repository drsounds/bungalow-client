#include "service/AccountManager.h"

namespace space {
namespace service {

void AccountManager::put(const SPSession& session) {
    if (!session.isValid()) return;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessions[session.serviceId()] = session;
    }
    notify();
}

bool AccountManager::remove(const std::string& serviceId) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_sessions.erase(serviceId) == 0) return false;
    }
    notify();
    return true;
}

std::optional<SPSession> AccountManager::get(const std::string& serviceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(serviceId);
    if (it == m_sessions.end()) return std::nullopt;
    return it->second;
}

bool AccountManager::isSignedIn(const std::string& serviceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_sessions.count(serviceId) != 0;
}

std::vector<SPSession> AccountManager::all() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<SPSession> result;
    result.reserve(m_sessions.size());
    for (const auto& entry : m_sessions) result.push_back(entry.second);
    return result;
}

void AccountManager::setChangedCallback(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_changed = std::move(callback);
}

void AccountManager::notify() {
    std::function<void()> callback;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        callback = m_changed;
    }
    if (callback) callback();
}

} // namespace service
} // namespace space
