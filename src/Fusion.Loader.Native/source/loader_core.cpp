
#include "fusion/loader/loader_core.hpp"

namespace fusion::loader
{
    bool LoaderCore::Initialize()
    {
        if (state_ == LoaderState::Ready)
        {
            return true;
        }

        if (state_ == LoaderState::ShuttingDown)
        {
            lastError_ = "Loader is shutting down.";
            state_ = LoaderState::Failed;
            return false;
        }

        lastError_.clear();
        state_ = LoaderState::Ready;

        return true;
    }

    void LoaderCore::Shutdown()
    {
        if (state_ == LoaderState::NotInitialized)
        {
            return;
        }

        state_ = LoaderState::ShuttingDown;
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
}
