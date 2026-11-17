
#include "fusion/loader/bootstrap_api.h"
#include "fusion/loader/loader_core.hpp"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace
{
    std::unique_ptr<fusion::loader::LoaderCore> g_loader;

    std::wstring g_lastError;

    void SetError(const std::string& message)
    {
        g_lastError.assign(message.begin(), message.end());
    }
}

extern "C" {

int FusionInitialize(
    const wchar_t* modsDirectory,
    const wchar_t* settingsFile)
{
    if (!modsDirectory || !settingsFile)
    {
        SetError("Fusion initialization paths cannot be null.");
        return 0;
    }

    try
    {
        if (g_loader &&
            g_loader->GetState() == fusion::loader::LoaderState::Ready)
        {
            return 1;
        }

        g_loader =
            std::make_unique<fusion::loader::LoaderCore>();

        const bool initialized = g_loader->Initialize(
            std::filesystem::path(modsDirectory),
            std::filesystem::path(settingsFile)
        );

        if (!initialized)
        {
            SetError(g_loader->GetLastError());
            g_loader.reset();
            return 0;
        }

        g_lastError.clear();
        return 1;
    }
    catch (const std::exception& exception)
    {
        SetError(exception.what());
        g_loader.reset();
        return 0;
    }
    catch (...)
    {
        SetError("Unknown error during Fusion initialization.");
        g_loader.reset();
        return 0;
    }
}

void FusionShutdown()
{
    if (g_loader)
    {
        g_loader->Shutdown();
        g_loader.reset();
    }
}

int FusionGetState()
{
    if (!g_loader)
        return static_cast<int>(
            fusion::loader::LoaderState::NotInitialized
        );

    return static_cast<int>(g_loader->GetState());
}

std::size_t FusionGetLastError(
    wchar_t* buffer,
    std::size_t bufferSize)
{
    const std::size_t required = g_lastError.size();

    if (buffer && bufferSize > 0)
    {
        const std::size_t copied =
            std::min(required, bufferSize - 1);

        std::copy_n(g_lastError.data(), copied, buffer);
        buffer[copied] = L'\0';
    }

    return required;
}

}
