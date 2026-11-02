#ifndef SWIMMING_CLI_H
#define SWIMMING_CLI_H

#include <optional>
#include <string>
#include <vector>

#include "swimming/Loader.h"

/// The swimming viewer's command line, shared by its platform hosts.
namespace swimming {
    constexpr const char* kCliUsage =
        "usage: swimming <bundle.js|index.html|URL> [--engine quickjs|v8] [--out frame.png] [--width N] [--height N]";

    struct CliOptions {
        std::string source;
        std::string engine; ///< JavaScript engine name (see engineNames()); empty for the default
        std::optional<std::string> out; ///< render one frame to this PNG instead of opening a window
        float width = 800;
        float height = 600;
    };

    /// The engine options.engine names, or the default one. Throws
    /// std::runtime_error if that engine is not built in.
    std::unique_ptr<JsEngine> createEngineFor(const CliOptions& options);

    /// Parses the arguments after the program name; nullopt without a source.
    std::optional<CliOptions> parseCli(const std::vector<std::string>& args);

    /// Loads the app headlessly and writes one frame (without glyphs) as a
    /// PNG. Throws std::runtime_error.
    void renderFrame(const CliOptions& options, const Fetch& fetch = {});
}

#endif
