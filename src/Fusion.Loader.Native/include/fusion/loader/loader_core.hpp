
#pragma once

#include <string>
#include "version.hpp"

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
        bool Initialize();
        void Shutdown();

        LoaderState GetState() const;
        const std::string& GetLastError() const;

        Version GetVersion() const;

    private:
        LoaderState state_ = LoaderState::NotInitialized;
        std::string lastError_;
    };
}
