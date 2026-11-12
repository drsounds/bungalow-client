#include "service/SessionCipher.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincrypt.h>
#endif

namespace space {
namespace service {

namespace {

#ifdef _WIN32
/// DPAPI: the key is derived from the signed-in Windows user's credentials.
class DpapiCipher : public SessionCipher {
public:
    bool encrypt(const std::string& plain, std::string& out) override {
        return transform(plain, out, true);
    }
    bool decrypt(const std::string& cipher, std::string& out) override {
        return transform(cipher, out, false);
    }

private:
    static bool transform(const std::string& in, std::string& out, bool protect) {
        DATA_BLOB input{static_cast<DWORD>(in.size()),
                        reinterpret_cast<BYTE*>(const_cast<char*>(in.data()))};
        DATA_BLOB result{};
        const BOOL ok = protect
            ? CryptProtectData(&input, L"Bungalow session", nullptr, nullptr, nullptr,
                               CRYPTPROTECT_UI_FORBIDDEN, &result)
            : CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr,
                                 CRYPTPROTECT_UI_FORBIDDEN, &result);
        if (!ok) return false;
        out.assign(reinterpret_cast<const char*>(result.pbData), result.cbData);
        if (!protect) SecureZeroMemory(result.pbData, result.cbData);
        LocalFree(result.pbData);
        return true;
    }
};
#endif

std::shared_ptr<SessionCipher>& slot() {
#ifdef _WIN32
    static std::shared_ptr<SessionCipher> cipher = std::make_shared<DpapiCipher>();
#else
    static std::shared_ptr<SessionCipher> cipher;
#endif
    return cipher;
}

} // namespace

SessionCipher* SessionCipher::instance() { return slot().get(); }

void SessionCipher::setInstance(std::shared_ptr<SessionCipher> cipher) { slot() = std::move(cipher); }

} // namespace service
} // namespace space
