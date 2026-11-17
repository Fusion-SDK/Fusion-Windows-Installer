
#include "fusion/loader/loader_core.hpp"

namespace fusion::loader
{
    bool LoaderCore::Initialize(
        const std::filesystem::path& modsDirectory,
        const std::filesystem::path& settingsFile)
    {
        if (state_ == LoaderState::Ready)
            return true;

        if (state_ == LoaderState::ShuttingDown)
        {
            lastError_ = "Loader is shutting down.";
            state_ = LoaderState::Failed;
            return false;
        }

        lastError_.clear();

        runtime_.Shutdown();
        loadPlan_.Clear();
        registry_.Clear();

        if (!settings_.Load(settingsFile, lastError_))
        {
            state_ = LoaderState::Failed;
            return false;
        }

        if (!registry_.Discover(modsDirectory))
        {
            lastError_ = "Unable to discover Fusion mods.";
            state_ = LoaderState::Failed;
            return false;
        }

        loadPlan_.BuildLoadPlan(registry_, settings_);

        // Individual mod failures do not prevent the loader itself
        // from becoming ready. Their results remain available through
        // GetRuntime().
        runtime_.LoadAll(loadPlan_, registry_);

        state_ = LoaderState::Ready;
        return true;
    }

    void LoaderCore::Shutdown()
    {
        if (state_ == LoaderState::NotInitialized)
            return;

        state_ = LoaderState::ShuttingDown;

        runtime_.Shutdown();
        loadPlan_.Clear();
        registry_.Clear();

        state_ = LoaderState::NotInitialized;
    }

    LoaderState LoaderCore::GetState() const
    {
        return state_;
    }

    const std::string& LoaderCore::GetLastError() const
    {
        return lastError_;
    }

    Version LoaderCore::GetVersion() const
    {
        return LoaderVersion;
    }

    const ModRegistry& LoaderCore::GetRegistry() const
    {
        return registry_;
    }

    const ModLoader& LoaderCore::GetLoadPlan() const
    {
        return loadPlan_;
    }

    const ModRuntime& LoaderCore::GetRuntime() const
    {
        return runtime_;
    }
}
