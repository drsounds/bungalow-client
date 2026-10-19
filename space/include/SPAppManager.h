#ifndef SP_APP_MANAGER_H
#define SP_APP_MANAGER_H

#include <string>
#include <vector>

#include "SPAppBundle.h"

namespace space {
    class SPSkin;

    /// Discovers app bundles and their extensions, and resolves sp:// URIs
    /// to files inside them.
    ///
    /// Bundles are directories containing a manifest.json. Search paths are
    /// scanned in the order they were added; a bundle found later replaces an
    /// earlier one with the same identifier, so user folders can override the
    /// bundled Apps.
    class SPAppManager {
    public:
        static SPAppManager& instance();

        /// Adds <base>/Apps, ~/Bungalow/Apps and <documents>/Bungalow/Apps.
        /// Empty arguments are skipped.
        void addDefaultSearchPaths(const std::string& base, const std::string& home,
                                   const std::string& documents);
        void addSearchPath(const std::string& dir);
        const std::vector<std::string>& getSearchPaths() const { return m_searchPaths; }

        /// Rescans all search paths.
        void scan();

        const std::vector<SPAppBundle>& getBundles() const { return m_bundles; }
        const SPAppBundle* findBundle(const std::string& identifier) const;

        /// All extensions of the given type across every bundle.
        std::vector<const SPAppExtension*> getExtensions(const std::string& type) const;
        const SPAppExtension* findExtension(const std::string& type, const std::string& identifier) const;

        /// Resolves sp://<bundle>/<path> to an absolute file path. Returns an
        /// empty string if the bundle is unknown or the path escapes it.
        std::string resolve(const std::string& uri) const;

        /// Resolves a path relative to a bundle directory. Returns an empty
        /// string if the path escapes the bundle. Thread-safe.
        static std::string resolveInBundle(const std::string& bundleRoot, const std::string& relativePath);

        /// Creates the skin for a skin type. Accepts a skin extension
        /// identifier ("modern.skin") or its short form ("modern"). Returns
        /// nullptr if no such skin bundle is installed or it fails to load.
        SPSkin* createSkin(const std::string& skinType) const;

    private:
        SPAppManager() = default;
        void scanDirectory(const std::string& dir);

        std::vector<std::string> m_searchPaths;
        std::vector<SPAppBundle> m_bundles;
    };
}

#endif
