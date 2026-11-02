#include "swimming/Cli.h"

#include <cstdlib>

namespace swimming {
    namespace {
        float parseSize(const std::string& value, float fallback) {
            char* end = nullptr;
            const float v = std::strtof(value.c_str(), &end);
            return end != value.c_str() && v > 0 ? v : fallback;
        }
    }

    std::optional<CliOptions> parseCli(const std::vector<std::string>& args) {
        CliOptions options;
        bool hasSource = false;
        for (size_t i = 0; i < args.size(); ++i) {
            const std::string& arg = args[i];
            const bool hasValue = i + 1 < args.size();
            if (arg == "--out" && hasValue) {
                options.out = args[++i];
            } else if (arg == "--width" && hasValue) {
                options.width = parseSize(args[++i], options.width);
            } else if (arg == "--height" && hasValue) {
                options.height = parseSize(args[++i], options.height);
            } else {
                options.source = arg;
                hasSource = true;
            }
        }
        if (!hasSource) return std::nullopt;
        return options;
    }

    void renderFrame(const CliOptions& options, const Fetch& fetch) {
        Bridge bridge(options.width, options.height);
        runSource(bridge, options.source, fetch);
        savePng(bridge.render(), options.out.value_or("frame.png"));
    }
}
