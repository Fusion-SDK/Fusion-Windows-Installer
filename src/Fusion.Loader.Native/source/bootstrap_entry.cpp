#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "fusion/loader/bootstrap_api.h"

#include <filesystem>
#include <string>
#include <cwchar>
#include <mutex>
#include <exception>

namespace {
    std::mutex g_logMutex;

    void WriteBootstrapLog(const std::filesystem::path& logPath, const wchar_t* message)
    {
        if (logPath.empty() || !message)
            return;

        SYSTEMTIME now{};
        GetLocalTime(&now);

        wchar_t timestamp[64]{};
        swprintf_s(timestamp, _countof(timestamp),
            L"[%04u-%02u-%02u %02u:%02u:%02u.%03u] ",
            now.wYear, now.wMonth, now.wDay,
            now.wHour, now.wMinute, now.wSecond, now.wMilliseconds);

        std::wstring line(timestamp);
        line += message;
        line += L"\r\n";

        const int required = WideCharToMultiByte(
            CP_UTF8, 0, line.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (required <= 1)
            return;

        std::string utf8(static_cast<std::size_t>(required), '\0');
        if (WideCharToMultiByte(
                CP_UTF8, 0, line.c_str(), -1, utf8.data(), required,
                nullptr, nullptr) == 0) {
            return;
        }
        utf8.resize(static_cast<std::size_t>(required - 1));

        std::lock_guard<std::mutex> lock(g_logMutex);
        HANDLE file = CreateFileW(
            logPath.c_str(),
            FILE_APPEND_DATA,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return;

        DWORD written = 0;
        WriteFile(file, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
        CloseHandle(file);
    }

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
            OutputDebugStringW(L"[Fusion] Could not resolve FusionBootstrap.dll path.\n");
            return 1;
        }

        DWORD result = 0;
        std::filesystem::path logPath;
        try {
            const std::filesystem::path fusionDirectory(modulePath);
            const auto root = fusionDirectory.parent_path();
            logPath = fusionDirectory.parent_path() / L"FusionBootstrap.log";

            WriteBootstrapLog(logPath, L"Fusion bootstrap worker started.");
            WriteBootstrapLog(logPath, L"Preparing Fusion mod and settings directories.");

            const auto mods = root / L"mods";
            const auto settings = root / L"mod-settings.json";
            std::error_code error;
            std::filesystem::create_directories(mods, error);
            if (error) {
                result = 2;
                const std::wstring message = L"Could not create the mods directory. Error: " +
                    std::to_wstring(error.value()) + L" (" + error.message().c_str() + L").";
                WriteBootstrapLog(logPath, message.c_str());
            }
            else {
                WriteBootstrapLog(logPath, L"Calling FusionInitialize.");
                result = FusionInitialize(mods.c_str(), settings.c_str()) ? 0 : 3;
                if (result == 0) {
                    WriteBootstrapLog(logPath, L"Fusion loader initialization completed successfully.");
                }
                else {
                    wchar_t lastError[1024]{};
                    FusionGetLastError(lastError, _countof(lastError));
                    WriteBootstrapLog(logPath, L"Fusion loader initialization failed.");
                    WriteBootstrapLog(logPath, lastError[0] ? lastError : L"No detailed error was provided.");
                }
            }
        }
        catch (const std::exception& exception) {
            result = 4;
            WriteBootstrapLog(logPath, L"An exception occurred during Fusion initialization.");
            OutputDebugStringA(exception.what());
        }
        catch (...) {
            result = 4;
            WriteBootstrapLog(logPath, L"An unknown exception occurred during Fusion initialization.");
        }

        if (result == 0) {
            WriteBootstrapLog(logPath, L"Bootstrap startup completed.");
            if (readyEvent) SetEvent(readyEvent);
        } else {
            WriteBootstrapLog(logPath, L"Bootstrap startup failed.");
            if (failedEvent) SetEvent(failedEvent);
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
        else
            OutputDebugStringW(L"[Fusion] Could not create the bootstrap worker thread.\n");
    }
    return TRUE;
}
#endif
