
#pragma once

#include <filesystem>
#include <string>

#include "version.hpp"
#include "mod_registry.hpp"
#include "mod_loader.hpp"
#include "mod_settings.hpp"
#include "mod_runtime.hpp"

namespace fusion::loader
{
    enum class LoaderState
    {
        NotInitialized,
        Ready,
        ShuttingDown,
        Failed
    };

    class LoaderCore
    {
    public:
        bool Initialize(
            const std::filesystem::path& modsDirectory,
            const std::filesystem::path& settingsFile
        );

        void Shutdown();

        LoaderState GetState() const;
        const std::string& GetLastError() const;

        Version GetVersion() const;

        const ModRegistry& GetRegistry() const;
        const ModLoader& GetLoadPlan() const;
        const ModRuntime& GetRuntime() const;

    private:
        LoaderState state_ = LoaderState::NotInitialized;
        std::string lastError_;

        ModRegistry registry_;
        ModSettingsStore settings_;
        ModLoader loadPlan_;
        ModRuntime runtime_;
    };
}
