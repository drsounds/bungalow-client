#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

namespace space {
namespace win {

struct HttpResult {
    bool        ok = false;          // transport succeeded and a response was read
    bool        cancelled = false;
    int         status = 0;
    int64_t     contentLength = -1;
    std::string contentType;
    std::string contentDisposition;
    std::string error;
};

// Blocking HTTP request, intended for worker threads. onData receives body chunks
// and returns false to abort. Follows redirects.
HttpResult httpRequest(const std::string& url,
                       const std::string& method,
                       const std::function<bool(const char*, size_t)>& onData,
                       const std::atomic<bool>* cancel = nullptr,
                       const std::function<void(const HttpResult&)>& onHeaders = nullptr);

} // namespace win
} // namespace space
