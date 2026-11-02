#include "SPSwimmingApp.h"

#include <filesystem>
#include <stdexcept>
#include <system_error>

namespace space {
    namespace fs = std::filesystem;

    bool SPSwimmingApp::isSwimming(const SPAppBundle& bundle) {
        const auto type = bundle.manifest.find("AppType");
        return type != bundle.manifest.end() && type->is_string() && type->get<std::string>() == "Swimming";
    }

    std::string SPSwimmingApp::entryPath(const SPAppBundle& bundle) {
        std::string main = kDefaultMain;
        const auto field = bundle.manifest.find("Main");
        if (field != bundle.manifest.end() && field->is_string())
            main = field->get<std::string>();

        const fs::path root = fs::u8path(bundle.path).lexically_normal();
        const fs::path entry = (root / fs::u8path(main)).lexically_normal();
        const fs::path inside = entry.lexically_relative(root);
        std::error_code ec;
        if (inside.empty() || *inside.begin() == ".." || !fs::is_regular_file(entry, ec))
            throw std::runtime_error("The app \"" + bundle.identifier + "\" has no " + main +
                                     "; build it with vite-plugin-swimming.");
        return entry.u8string();
    }
}
