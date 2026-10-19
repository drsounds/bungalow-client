#ifndef SP_APP_BUNDLE_H
#define SP_APP_BUNDLE_H

#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace space {
    /// An extension contributed by an app bundle, e.g. a skin found in
    /// <bundle>/skins/<name>/manifest.json.
    struct SPAppExtension {
        std::string type;        ///< Manifest BundleType, e.g. "Skin"
        std::string identifier;  ///< Manifest BundleIdentifier, e.g. "modern.skin"
        std::string path;        ///< Absolute directory of the extension
        std::string bundleId;    ///< Identifier of the owning app bundle
        std::map<std::string, std::string> names;
        std::map<std::string, std::string> descriptions;
        nlohmann::json manifest;

        std::string getName(const std::string& lang = "en") const;
        std::string getDescription(const std::string& lang = "en") const;
    };

    /// An app bundle: a directory with a manifest.json, found in one of the
    /// app search paths (see SPAppManager).
    struct SPAppBundle {
        std::string type;        ///< Manifest BundleType, e.g. "Application"
        std::string identifier;  ///< Manifest BundleIdentifier; host of sp:// URIs
        std::string path;        ///< Absolute directory of the bundle
        std::map<std::string, std::string> names;
        std::map<std::string, std::string> descriptions;
        nlohmann::json manifest;
        std::vector<SPAppExtension> extensions;

        std::string getName(const std::string& lang = "en") const;
        std::string getDescription(const std::string& lang = "en") const;
    };
}

#endif
