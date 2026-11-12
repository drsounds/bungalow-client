#pragma once

#include <memory>
#include <string>

namespace space {
namespace service {

/// Encrypts the data an SPSession keeps. The implementation is the platform's
/// own secret storage (Windows: DPAPI, so only the same OS user can decrypt),
/// installed with setInstance(); tests install their own.
class SessionCipher {
public:
    virtual ~SessionCipher() = default;

    /// Returns false (leaving `out` untouched) when it cannot encrypt.
    virtual bool encrypt(const std::string& plain, std::string& out) = 0;
    /// Returns false when `cipher` is corrupt or belongs to another user.
    virtual bool decrypt(const std::string& cipher, std::string& out) = 0;

    /// The cipher in use, or nullptr on a platform without one: sessions
    /// then refuse to hold data rather than keeping it in the clear.
    static SessionCipher* instance();
    static void setInstance(std::shared_ptr<SessionCipher> cipher);
};

} // namespace service
} // namespace space
