#include "plugin/PluginManager.h"

#include "plugin/ComServiceAdapter.h"
#include "service/ServiceManager.h"

#include <algorithm>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace space {
namespace plugin {

using namespace com;
namespace fs = std::filesystem;

namespace {

// ── Shared libraries ─────────────────────────────────────────────────────────

#ifdef _WIN32
const char* const kLibraryExtension = ".dll";

void* openLibrary(const fs::path& file) {
    // Its own directory first, so a plugin can ship the DLLs it depends on.
    return LoadLibraryExW(fs::absolute(file).c_str(), nullptr,
                          LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
}
void closeLibrary(void* library) { FreeLibrary(static_cast<HMODULE>(library)); }
void* findSymbol(void* library, const char* name) {
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(library), name));
}
#else
#ifdef __APPLE__
const char* const kLibraryExtension = ".dylib";
#else
const char* const kLibraryExtension = ".so";
#endif

void* openLibrary(const fs::path& file) { return dlopen(file.c_str(), RTLD_NOW | RTLD_LOCAL); }
void closeLibrary(void* library) { dlclose(library); }
void* findSymbol(void* library, const char* name) { return dlsym(library, name); }
#endif

bool isLibrary(const fs::path& file) {
    std::string ext = file.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });
    return ext == kLibraryExtension;
}

bool fail(std::string* error, std::string message) {
    if (error) *error = std::move(message);
    return false;
}

} // namespace

// ── Entry ────────────────────────────────────────────────────────────────────

struct PluginManager::Entry {
    PluginInfo            info;
    ComPtr<IApplication>  application;
    void*                 library = nullptr; // null for Builtin
};

// ── The recording context ────────────────────────────────────────────────────

class PluginManager::Context final : public ComObject<IApplicationContext> {
public:
    Context(PluginManager* manager, Entry* entry) : m_manager(manager), m_entry(entry) {}

    /// After OnActivate returned: a plugin that kept the context gets errors.
    void close() {
        m_manager = nullptr;
        m_entry = nullptr;
    }

    Result BUNGALOW_CALL GetApplicationId(IString** out) override {
        if (!m_entry) return kFail;
        return returnString(m_entry->info.id, out);
    }
    Result BUNGALOW_CALL RegisterService(IService* service) override {
        if (!m_manager) return kFail;
        return m_manager->registerService(*m_entry, service);
    }
    // Views cannot cross the plugin boundary yet; see BungalowPlugin.h.
    Result BUNGALOW_CALL RegisterFeature(IFeature*) override { return kNotImpl; }
    Result BUNGALOW_CALL RegisterConcept(IConcept*) override { return kNotImpl; }

private:
    PluginManager* m_manager;
    Entry*         m_entry;
};

// ── PluginManager ────────────────────────────────────────────────────────────

PluginManager::PluginManager(service::ServiceManager& services, service::Executor ui)
    : m_services(services), m_ui(std::move(ui)) {}

PluginManager::~PluginManager() {
    unloadAll();
}

int PluginManager::loadDirectory(const fs::path& dir, PluginSource source) {
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) return 0;

    std::vector<fs::path> files;
    for (const fs::directory_entry& item : fs::directory_iterator(dir, ec)) {
        if (item.is_regular_file(ec) && isLibrary(item.path())) files.push_back(item.path());
    }
    std::sort(files.begin(), files.end());

    int loaded = 0;
    for (const fs::path& file : files)
        if (loadLibrary(file, source)) ++loaded;
    return loaded;
}

bool PluginManager::loadLibrary(const fs::path& file, PluginSource source, std::string* error) {
    void* library = openLibrary(file);
    if (!library) return fail(error, "The library could not be loaded");

    auto create = reinterpret_cast<CreateApplicationFn>(findSymbol(library, kCreateApplicationSymbol));
    if (!create) {
        closeLibrary(library);
        return fail(error, "Not a Bungalow plugin");
    }

    ComPtr<IApplication> application;
    if (failed(create(kAbiVersion, application.put())) || !application) {
        application.reset();
        closeLibrary(library);
        return fail(error, "The plugin was built for another version of Bungalow");
    }

    if (!add(std::move(application), source, file, library, error)) {
        // add() released the application; nothing of the library is in use.
        closeLibrary(library);
        return false;
    }
    return true;
}

bool PluginManager::addApplication(ComPtr<IApplication> application, std::string* error, PluginSource source,
                                   const fs::path& path) {
    return add(std::move(application), source, path, nullptr, error);
}

bool PluginManager::add(ComPtr<IApplication> application, PluginSource source, const fs::path& path,
                        void* library, std::string* error) {
    if (!application) return fail(error, "No application");

    auto entry = std::make_unique<Entry>();
    entry->info.id = getString([&](IString** o) { return application->GetId(o); });
    if (entry->info.id.empty()) return fail(error, "The plugin has no id");
    if (find(entry->info.id)) return fail(error, "A plugin with the id " + entry->info.id + " is already loaded");

    entry->info.name = getString([&](IString** o) { return application->GetName(o); });
    if (entry->info.name.empty()) entry->info.name = entry->info.id;
    entry->info.version = getString([&](IString** o) { return application->GetVersion(o); });
    if (entry->info.version.empty()) entry->info.version = "0.0.0";
    entry->info.source = source;
    entry->info.path = path;
    entry->application = std::move(application);
    entry->library = library;

    if (!activate(*entry)) {
        entry->library = nullptr; // the caller closes it
        return fail(error, "The plugin failed to activate");
    }
    m_entries.push_back(std::move(entry));
    return true;
}

PluginManager::Entry* PluginManager::find(const std::string& id) const {
    for (const auto& entry : m_entries)
        if (entry->info.id == id) return entry.get();
    return nullptr;
}

bool PluginManager::activate(Entry& entry) {
    if (entry.info.active) return true;
    auto context = make<Context>(this, &entry);
    const Result result = entry.application->OnActivate(context.get());
    context->close();
    entry.info.active = true;
    if (failed(result)) {
        deactivate(entry); // undo what it registered before failing
        return false;
    }
    return true;
}

void PluginManager::deactivate(Entry& entry) {
    if (!entry.info.active) return;
    // Newest first, mirroring registration.
    for (auto it = entry.info.serviceIds.rbegin(); it != entry.info.serviceIds.rend(); ++it) {
        m_services.accounts().remove(*it);
        m_services.unregisterAspect(*it); // onStop, onDestroy, then releases the plugin's object
    }
    entry.info.serviceIds.clear();
    entry.application->OnDeactivate();
    entry.info.active = false;
}

Result PluginManager::registerService(Entry& entry, IService* service) {
    if (!service) return kPointer;
    std::unique_ptr<service::Service> adapter = wrapService(ComPtr<IService>(service), m_ui);
    if (!adapter) return kInvalidArg;

    // A plugin may not replace a service that is already there.
    const std::string id = adapter->getId();
    if (m_services.get(id)) return kFail;

    service::Service* registered = m_services.registerAspect(std::move(adapter));
    if (!registered) return kFail;
    entry.info.serviceIds.push_back(id);
    if (m_services.isStarted()) registered->onStart();
    return kOk;
}

bool PluginManager::setEnabled(const std::string& id, bool enabled) {
    Entry* entry = find(id);
    if (!entry) return false;
    if (enabled) return activate(*entry);
    deactivate(*entry);
    return true;
}

std::vector<PluginInfo> PluginManager::plugins() const {
    std::vector<PluginInfo> result;
    result.reserve(m_entries.size());
    for (const auto& entry : m_entries) result.push_back(entry->info);
    return result;
}

void PluginManager::unloadAll() {
    std::vector<void*> libraries;
    for (auto it = m_entries.rbegin(); it != m_entries.rend(); ++it) {
        Entry& entry = **it;
        deactivate(entry);
        entry.application.reset();
        if (entry.library) libraries.push_back(entry.library);
    }
    m_entries.clear();
    // Last: by now no object of any plugin is referenced by the host.
    for (void* library : libraries) closeLibrary(library);
}

} // namespace plugin
} // namespace space
