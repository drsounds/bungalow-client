#include "SPAppManager.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "SPXmlSkin.h"

namespace fs = std::filesystem;

namespace space {
    namespace {
        const char* kSpScheme = "sp://";

        // Folders inside an app bundle that hold extensions, and the
        // BundleType their manifests must declare.
        const std::pair<const char*, const char*> kExtensionFolders[] = {
            { "skins", "Skin" },
        };

        bool readJson(const fs::path& file, nlohmann::json& out) {
            std::ifstream in(file);
            if (!in.is_open()) return false;
            std::ostringstream ss;
            ss << in.rdbuf();
            try {
                out = nlohmann::json::parse(ss.str());
            } catch (const std::exception& e) {
                std::cerr << "Invalid manifest " << file.u8string() << ": " << e.what() << std::endl;
                return false;
            }
            return out.is_object();
        }

        std::string stringField(const nlohmann::json& obj, const char* key) {
            auto it = obj.find(key);
            return (it != obj.end() && it->is_string()) ? it->get<std::string>() : std::string();
        }

        // Reads a localized field such as {"AppName": {"en": "..."}}; a plain
        // string is stored under "en".
        std::map<std::string, std::string> localizedField(const nlohmann::json& obj, const char* key) {
            std::map<std::string, std::string> result;
            auto it = obj.find(key);
            if (it == obj.end()) return result;
            if (it->is_string()) {
                result["en"] = it->get<std::string>();
            } else if (it->is_object()) {
                for (auto& [lang, value] : it->items())
                    if (value.is_string()) result[lang] = value.get<std::string>();
            }
            return result;
        }

        std::string localized(const std::map<std::string, std::string>& values, const std::string& lang) {
            auto it = values.find(lang);
            if (it != values.end()) return it->second;
            it = values.find("en");
            if (it != values.end()) return it->second;
            return values.empty() ? std::string() : values.begin()->second;
        }

        std::vector<fs::path> subdirectories(const fs::path& dir) {
            std::vector<fs::path> result;
            std::error_code ec;
            for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec))
                if (it->is_directory(ec)) result.push_back(it->path());
            std::sort(result.begin(), result.end());
            return result;
        }
    }

    std::string SPAppExtension::getName(const std::string& lang) const {
        std::string name = localized(names, lang);
        return name.empty() ? identifier : name;
    }

    std::string SPAppExtension::getDescription(const std::string& lang) const {
        return localized(descriptions, lang);
    }

    std::string SPAppBundle::getName(const std::string& lang) const {
        std::string name = localized(names, lang);
        return name.empty() ? identifier : name;
    }

    std::string SPAppBundle::getDescription(const std::string& lang) const {
        return localized(descriptions, lang);
    }

    SPAppManager& SPAppManager::instance() {
        static SPAppManager manager;
        return manager;
    }

    void SPAppManager::addDefaultSearchPaths(const std::string& base, const std::string& home,
                                             const std::string& documents) {
        if (!base.empty())
            addSearchPath((fs::u8path(base) / "Apps").u8string());
        if (!home.empty())
            addSearchPath((fs::u8path(home) / "Bungalow" / "Apps").u8string());
        if (!documents.empty())
            addSearchPath((fs::u8path(documents) / "Bungalow" / "Apps").u8string());
    }

    void SPAppManager::addSearchPath(const std::string& dir) {
        std::string normalized = fs::u8path(dir).lexically_normal().u8string();
        if (std::find(m_searchPaths.begin(), m_searchPaths.end(), normalized) == m_searchPaths.end())
            m_searchPaths.push_back(normalized);
    }

    void SPAppManager::scan() {
        m_bundles.clear();
        for (const std::string& dir : m_searchPaths)
            scanDirectory(dir);
    }

    void SPAppManager::scanDirectory(const std::string& dir) {
        for (const fs::path& bundleDir : subdirectories(fs::u8path(dir))) {
            nlohmann::json manifest;
            if (!readJson(bundleDir / "manifest.json", manifest))
                continue;

            SPAppBundle bundle;
            bundle.manifest = manifest;
            bundle.type = stringField(manifest, "BundleType");
            bundle.identifier = stringField(manifest, "BundleIdentifier");
            if (bundle.identifier.empty())
                bundle.identifier = bundleDir.filename().u8string();
            bundle.path = fs::absolute(bundleDir).lexically_normal().u8string();
            bundle.names = localizedField(manifest, "AppName");
            bundle.descriptions = localizedField(manifest, "AppDescription");

            for (const auto& [folder, type] : kExtensionFolders) {
                for (const fs::path& extDir : subdirectories(bundleDir / folder)) {
                    nlohmann::json extManifest;
                    if (!readJson(extDir / "manifest.json", extManifest))
                        continue;
                    SPAppExtension ext;
                    ext.manifest = extManifest;
                    ext.type = stringField(extManifest, "BundleType");
                    if (ext.type.empty()) ext.type = type;
                    if (ext.type != type) {
                        std::cerr << "Skipping " << extDir.u8string() << ": BundleType "
                                  << ext.type << " does not belong in " << folder << std::endl;
                        continue;
                    }
                    ext.identifier = stringField(extManifest, "BundleIdentifier");
                    if (ext.identifier.empty())
                        ext.identifier = bundle.identifier + "." + extDir.filename().u8string();
                    ext.path = fs::absolute(extDir).lexically_normal().u8string();
                    ext.bundleId = bundle.identifier;
                    // Extension manifests name themselves after their type,
                    // e.g. SkinName / SkinDescription.
                    ext.names = localizedField(extManifest, (ext.type + "Name").c_str());
                    ext.descriptions = localizedField(extManifest, (ext.type + "Description").c_str());
                    bundle.extensions.push_back(std::move(ext));
                }
            }

            auto existing = std::find_if(m_bundles.begin(), m_bundles.end(),
                [&](const SPAppBundle& b) { return b.identifier == bundle.identifier; });
            if (existing != m_bundles.end())
                *existing = std::move(bundle);
            else
                m_bundles.push_back(std::move(bundle));
        }
    }

    const SPAppBundle* SPAppManager::findBundle(const std::string& identifier) const {
        for (const SPAppBundle& bundle : m_bundles)
            if (bundle.identifier == identifier) return &bundle;
        return nullptr;
    }

    std::vector<const SPAppExtension*> SPAppManager::getExtensions(const std::string& type) const {
        std::vector<const SPAppExtension*> result;
        for (const SPAppBundle& bundle : m_bundles)
            for (const SPAppExtension& ext : bundle.extensions)
                if (ext.type == type) result.push_back(&ext);
        return result;
    }

    const SPAppExtension* SPAppManager::findExtension(const std::string& type, const std::string& identifier) const {
        for (const SPAppExtension* ext : getExtensions(type))
            if (ext->identifier == identifier) return ext;
        return nullptr;
    }

    std::string SPAppManager::resolve(const std::string& uri) const {
        const size_t schemeLen = std::char_traits<char>::length(kSpScheme);
        if (uri.compare(0, schemeLen, kSpScheme) != 0)
            return {};
        const size_t slash = uri.find('/', schemeLen);
        const std::string host = uri.substr(schemeLen, slash == std::string::npos ? std::string::npos : slash - schemeLen);
        const std::string rest = slash == std::string::npos ? std::string() : uri.substr(slash + 1);

        const SPAppBundle* bundle = findBundle(host);
        if (!bundle) return {};

        const fs::path root = fs::u8path(bundle->path);
        const fs::path target = (root / fs::u8path(rest)).lexically_normal();
        // Reject paths that climb out of the bundle (sp://app/../../secret).
        const fs::path rel = target.lexically_relative(root);
        if (rel.empty() || *rel.begin() == "..")
            return {};
        return target.u8string();
    }

    SPSkin* SPAppManager::createSkin(const std::string& skinType) const {
        const SPAppExtension* ext = findExtension("Skin", skinType);
        if (!ext) ext = findExtension("Skin", skinType + ".skin");
        if (!ext) return nullptr;

        auto* skin = new SPXmlSkin();
        if (!skin->load((fs::u8path(ext->path) / "skin.xml").u8string(), ext->path)) {
            delete skin;
            return nullptr;
        }
        return skin;
    }
}
