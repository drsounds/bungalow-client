#ifndef SP_SWIMMING_APP_H
#define SP_SWIMMING_APP_H

#include <string>

#include "SPAppBundle.h"

namespace space {
    /// An app bundle with "AppType": "Swimming": a React (TSX) app built
    /// with vite-plugin-swimming and run natively by the swimming library
    /// (see SPSwimmingAppElement), with no browser engine.
    ///
    /// The manifest's "Main" names the entry, relative to the bundle
    /// directory: the Vite IIFE bundle (default "dist/bundle.js") or an .html
    /// file whose classic scripts are run.
    class SPSwimmingApp {
    public:
        static constexpr const char* kDefaultMain = "dist/bundle.js";

        static bool isSwimming(const SPAppBundle& bundle);

        /// The entry file's path. Throws std::runtime_error if it is missing
        /// or outside the bundle.
        static std::string entryPath(const SPAppBundle& bundle);
    };
}

#endif
