#pragma once

#include "SPUser.h"

#include <nlohmann/json_fwd.hpp>

#include <string>

/// A sign-in with a back-end service, connected to the SPUser it belongs to.
/// What the service needs to resume it (tokens, cookies, …) is kept as JSON
/// that is encrypted with the service::SessionCipher the moment it is set, so
/// the clear text never lives in the session. A default-constructed session is
/// the "signed out" session.
class SPSession {
public:
    SPSession() = default;
    SPSession(SPUser user, std::string serviceId);

    const SPUser&      user() const { return m_user; }
    /// Id of the Service that issued the session.
    const std::string& serviceId() const { return m_serviceId; }
    bool isValid() const { return m_user.isValid(); }

    /// Encrypts `data` into the session. False (session unchanged) when no
    /// cipher is available or encryption failed.
    bool setData(const nlohmann::json& data);
    /// Decrypts the data. `ok`, when given, tells "no data / cannot decrypt"
    /// (null JSON, *ok == false) from a stored JSON null.
    nlohmann::json data(bool* ok = nullptr) const;
    bool hasData() const { return !m_encrypted.empty(); }

    /// The encrypted blob, to persist; restore it with setEncryptedData().
    const std::string& encryptedData() const { return m_encrypted; }
    void setEncryptedData(std::string blob) { m_encrypted = std::move(blob); }

private:
    SPUser      m_user;
    std::string m_serviceId;
    std::string m_encrypted;
};
