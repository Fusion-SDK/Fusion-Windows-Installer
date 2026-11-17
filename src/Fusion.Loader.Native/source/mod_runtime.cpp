#include "fusion/loader/mod_runtime.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>

#include "fusion/sdk/plugin_api.h"

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace fusion::loader {

    namespace {

        void HostLog(std::uint32_t level, const char* message)
        {
            const char* label = "INFO";
            if (level == FUSION_LOG_WARNING) label = "WARNING";
            else if (level == FUSION_LOG_ERROR) label = "ERROR";

            std::cerr << "[Fusion Mod " << label << "] "
                      << (message ? message : "") << '\n';
        }

        const FusionHostApi HostApi{
            sizeof(FusionHostApi),
            FUSION_MOD_API_VERSION,
            &HostLog
        };

        bool IsPathInside(
            const std::filesystem::path& child,
            const std::filesystem::path& parent)
        {
            auto childIt = child.begin();
            auto parentIt = parent.begin();

            for (; parentIt != parent.end(); ++parentIt, ++childIt) {
                if (childIt == child.end() || *childIt != *parentIt)
                    return false;
            }
            return true;
        }

        bool ResolveLibraryPath(
            const DiscoveredMod& mod,
            std::filesystem::path& result,
            std::string& error)
        {
            if (mod.metadata.library.empty()) {
                error = "Manifest does not specify a native library.";
                return false;
            }

            const std::filesystem::path relative(mod.metadata.library);
            if (relative.is_absolute()) {
                error = "Native library path must be relative to the mod directory.";
                return false;
            }

            std::error_code ec;
            const auto root = std::filesystem::weakly_canonical(mod.directory, ec);
            if (ec) {
                error = "Unable to resolve mod directory.";
                return false;
            }

            const auto candidate = std::filesystem::weakly_canonical(root / relative, ec);
            if (ec || !IsPathInside(candidate, root)) {
                error = "Native library path escapes the mod directory or is invalid.";
                return false;
            }

            if (!std::filesystem::is_regular_file(candidate, ec) || ec) {
                error = "Native library file does not exist.";
                return false;
            }

            result = candidate;
            return true;
        }

        void* OpenLibrary(const std::filesystem::path& path, std::string& error)
        {
#if defined(_WIN32)
            HMODULE module = LoadLibraryW(path.c_str());
            if (!module) {
                error = "Windows could not load the native library (error " +
                    std::to_string(GetLastError()) + ").";
            }
            return reinterpret_cast<void*>(module);
#else
            void* module = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
            if (!module) {
                const char* detail = dlerror();
                error = detail ? detail : "Unable to load native library.";
            }
            return module;
#endif
        }

        void* FindSymbol(void* handle, const char* name)
        {
#if defined(_WIN32)
            return reinterpret_cast<void*>(GetProcAddress(
                reinterpret_cast<HMODULE>(handle), name));
#else
            dlerror();
            return dlsym(handle, name);
#endif
        }

        void CloseLibrary(void* handle)
        {
            if (!handle) return;
#if defined(_WIN32)
            FreeLibrary(reinterpret_cast<HMODULE>(handle));
#else
            dlclose(handle);
#endif
        }

    }

    ModRuntime::~ModRuntime()
    {
        Shutdown();
    }

    bool ModRuntime::LoadAll(
        const ModLoader& loadPlan,
        const ModRegistry& registry)
    {
        Shutdown();
        results_.clear();

        // Surface disabled and validation-failed entries in the runtime
        // status list. Only IDs in the load order are eligible to load.
        for (const auto& managed : loadPlan.GetManagedMods()) {
            if (managed.state == ModState::Disabled) {
                results_.push_back(RuntimeModResult{
                    managed.discovered.metadata.id,
                    RuntimeModState::Disabled,
                    managed.statusMessage
                });
            }
            else if (managed.state == ModState::Failed) {
                results_.push_back(RuntimeModResult{
                    managed.discovered.metadata.id,
                    RuntimeModState::Failed,
                    managed.statusMessage
                });
            }
        }

        for (const auto& id : loadPlan.GetLoadOrder()) {
            const auto found = std::find_if(
                registry.GetMods().begin(), registry.GetMods().end(),
                [&id](const DiscoveredMod& mod) {
                    return mod.valid && mod.metadata.id == id;
                });

            RuntimeModResult result;
            result.id = id;

            if (found == registry.GetMods().end()) {
                result.statusMessage = "Mod disappeared after load planning.";
                results_.push_back(std::move(result));
                continue;
            }

            bool dependenciesLoaded = true;
            for (const auto& dependency : found->metadata.dependencies) {
                const bool loaded = std::any_of(
                    loaded_.begin(), loaded_.end(),
                    [&dependency](const LoadedModule& module) {
                        return module.id == dependency.id;
                    });
                if (!loaded) {
                    result.statusMessage =
                        "Dependency did not initialize successfully: " +
                        dependency.id;
                    dependenciesLoaded = false;
                    break;
                }
            }
            if (!dependenciesLoaded) {
                results_.push_back(std::move(result));
                continue;
            }

            std::filesystem::path libraryPath;
            if (!ResolveLibraryPath(*found, libraryPath, result.statusMessage)) {
                results_.push_back(std::move(result));
                continue;
            }

            std::string loadError;
            void* handle = OpenLibrary(libraryPath, loadError);
            if (!handle) {
                result.statusMessage = std::move(loadError);
                results_.push_back(std::move(result));
                continue;
            }

            auto getApi = reinterpret_cast<FusionModGetApiFn>(
                FindSymbol(handle, "FusionModGetApi"));
            if (!getApi) {
                result.statusMessage = "Native library is missing FusionModGetApi.";
                CloseLibrary(handle);
                results_.push_back(std::move(result));
                continue;
            }

            const FusionModApi* api = getApi();
            if (!api || api->struct_size < sizeof(FusionModApi) ||
                api->api_version != FUSION_MOD_API_VERSION ||
                api->info.struct_size < sizeof(FusionModInfo) ||
                api->info.required_api_version != FUSION_MOD_API_VERSION ||
                !api->info.id || !api->info.name || !api->info.version ||
                !api->info.author || !api->on_load || !api->on_unload) {
                result.statusMessage = "Native module returned an incompatible or incomplete API.";
                CloseLibrary(handle);
                results_.push_back(std::move(result));
                continue;
            }

            if (found->metadata.id != api->info.id ||
                found->metadata.name != api->info.name ||
                found->metadata.version != api->info.version ||
                found->metadata.author != api->info.author) {
                result.statusMessage = "Native module information does not match mod.json.";
                CloseLibrary(handle);
                results_.push_back(std::move(result));
                continue;
            }

            if (api->on_load(&HostApi) == 0) {
                result.statusMessage = "Mod initialization was rejected.";
                CloseLibrary(handle);
                results_.push_back(std::move(result));
                continue;
            }

            loaded_.push_back(LoadedModule{
                id,
                handle,
                api->on_unload
            });
            result.state = RuntimeModState::Loaded;
            result.statusMessage = "Mod initialized successfully.";
            results_.push_back(std::move(result));
        }

        running_ = !loaded_.empty();
        return std::all_of(results_.begin(), results_.end(),
            [](const RuntimeModResult& result) {
                return result.state == RuntimeModState::Loaded ||
                    result.state == RuntimeModState::Disabled;
            });
    }

    void ModRuntime::Shutdown()
    {
        for (auto it = loaded_.rbegin(); it != loaded_.rend(); ++it) {
            if (it->unload) {
                it->unload();
            }
            CloseLibrary(it->handle);
        }
        loaded_.clear();
        running_ = false;
    }

    bool ModRuntime::IsRunning() const
    {
        return running_;
    }

    const std::vector<RuntimeModResult>& ModRuntime::GetResults() const
    {
        return results_;
    }

}
