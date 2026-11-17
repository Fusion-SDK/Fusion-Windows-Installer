#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "fusion/loader/bootstrap_api.h"

#include <filesystem>
#include <string>
#include <cwchar>

namespace {
    DWORD WINAPI InitializeFusion(void* moduleHandle)
    {
        wchar_t eventName[96]{};
        swprintf_s(eventName, _countof(eventName), L"Local\\FusionBootstrapReady_%lu", GetCurrentProcessId());
        HANDLE readyEvent = CreateEventW(nullptr, TRUE, FALSE, eventName);
        wchar_t failedEventName[112]{};
        swprintf_s(failedEventName, _countof(failedEventName),
            L"Local\\FusionBootstrapFailed_%lu", GetCurrentProcessId());
        HANDLE failedEvent = CreateEventW(nullptr, TRUE, FALSE, failedEventName);

        wchar_t modulePath[MAX_PATH]{};
        const auto module = static_cast<HMODULE>(moduleHandle);
        const DWORD length = GetModuleFileNameW(module, modulePath, MAX_PATH);
        if (length == 0 || length >= MAX_PATH) {
            if (failedEvent) SetEvent(failedEvent);
            if (readyEvent) CloseHandle(readyEvent);
            if (failedEvent) CloseHandle(failedEvent);
            return 1;
        }

        DWORD result = 0;
        try {
            const std::filesystem::path fusionDirectory(modulePath);
            const auto root = fusionDirectory.parent_path();
            const auto mods = root / L"mods";
            const auto settings = root / L"mod-settings.json";
            std::error_code error;
            std::filesystem::create_directories(mods, error);
            if (error)
                result = 2;
            else
                result = FusionInitialize(mods.c_str(), settings.c_str()) ? 0 : 3;
        }
        catch (...) {
            result = 4;
        }

        if (result == 0) {
            if (readyEvent) SetEvent(readyEvent);
        } else if (failedEvent) {
            SetEvent(failedEvent);
        }
        if (readyEvent) CloseHandle(readyEvent);
        if (failedEvent) CloseHandle(failedEvent);
        return result;
    }
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        // Initialization is deferred to a worker thread so the loader does
        // not perform filesystem work or load mod DLLs under the loader lock.
        HANDLE thread = CreateThread(nullptr, 0, InitializeFusion, instance, 0, nullptr);
        if (thread)
            CloseHandle(thread);
    }
    return TRUE;
}
#endif
