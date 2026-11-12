#include "models/SPSession.h"

#include "service/SessionCipher.h"

#include <nlohmann/json.hpp>

using space::service::SessionCipher;

SPSession::SPSession(SPUser user, std::string serviceId)
    : m_user(std::move(user)), m_serviceId(std::move(serviceId)) {}

bool SPSession::setData(const nlohmann::json& data) {
    SessionCipher* cipher = SessionCipher::instance();
    if (!cipher) return false;
    std::string encrypted;
    if (!cipher->encrypt(data.dump(), encrypted)) return false;
    m_encrypted = std::move(encrypted);
    return true;
}

nlohmann::json SPSession::data(bool* ok) const {
    if (ok) *ok = false;
    SessionCipher* cipher = SessionCipher::instance();
    std::string plain;
    if (m_encrypted.empty() || !cipher || !cipher->decrypt(m_encrypted, plain)) return nullptr;
    nlohmann::json parsed = nlohmann::json::parse(plain, nullptr, false);
    if (parsed.is_discarded()) return nullptr;
    if (ok) *ok = true;
    return parsed;
}
