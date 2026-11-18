#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

struct XINPUT_GAMEPAD { WORD wButtons; BYTE bLeftTrigger; BYTE bRightTrigger; SHORT sThumbLX; SHORT sThumbLY; SHORT sThumbRX; SHORT sThumbRY; };
struct XINPUT_STATE { DWORD dwPacketNumber; XINPUT_GAMEPAD Gamepad; };
struct XINPUT_VIBRATION { WORD wLeftMotorSpeed; WORD wRightMotorSpeed; };
struct XINPUT_CAPABILITIES { BYTE Type; BYTE SubType; WORD Flags; XINPUT_GAMEPAD Gamepad; XINPUT_VIBRATION Vibration; };
struct XINPUT_BATTERY_INFORMATION { BYTE BatteryType; BYTE BatteryLevel; };
struct XINPUT_KEYSTROKE { WORD VirtualKey; WCHAR Unicode; WORD Flags; BYTE UserIndex; BYTE HidCode; };

namespace {
HMODULE g_systemXInput = nullptr;
HMODULE g_fusionBootstrap = nullptr;
INIT_ONCE g_resolveOnce = INIT_ONCE_STATIC_INIT;

using GetStateFn = DWORD (WINAPI*)(DWORD, XINPUT_STATE*);
using SetStateFn = DWORD (WINAPI*)(DWORD, XINPUT_VIBRATION*);
using GetCapabilitiesFn = DWORD (WINAPI*)(DWORD, DWORD, XINPUT_CAPABILITIES*);
using EnableFn = void (WINAPI*)(BOOL);
using GetAudioDeviceIdsFn = DWORD (WINAPI*)(DWORD, LPWSTR, UINT*, LPWSTR, UINT*);
using GetBatteryInformationFn = DWORD (WINAPI*)(DWORD, BYTE, XINPUT_BATTERY_INFORMATION*);
using GetKeystrokeFn = DWORD (WINAPI*)(DWORD, DWORD, XINPUT_KEYSTROKE*);

GetStateFn g_getState = nullptr;
GetStateFn g_getStateEx = nullptr;
SetStateFn g_setState = nullptr;
GetCapabilitiesFn g_getCapabilities = nullptr;
EnableFn g_enable = nullptr;
GetAudioDeviceIdsFn g_getAudioDeviceIds = nullptr;
GetBatteryInformationFn g_getBatteryInformation = nullptr;
GetKeystrokeFn g_getKeystroke = nullptr;

void WriteProxyLog(const std::wstring& gameDirectory, const wchar_t* message) {
    std::wstring logPath = gameDirectory;
    logPath += L"Fusion\\FusionProxy.log";
    HANDLE file = CreateFileW(logPath.c_str(), FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;

    SYSTEMTIME now{};
    GetLocalTime(&now);
    wchar_t line[768]{};
    swprintf_s(line, L"[%04u-%02u-%02u %02u:%02u:%02u] %s\r\n",
        now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute,
        now.wSecond, message ? message : L"");
    const int required = WideCharToMultiByte(CP_UTF8, 0, line, -1, nullptr, 0, nullptr, nullptr);
    if (required > 1) {
        std::string utf8(static_cast<std::size_t>(required), '\0');
        if (WideCharToMultiByte(CP_UTF8, 0, line, -1, utf8.data(), required, nullptr, nullptr) > 0) {
            DWORD written = 0;
            WriteFile(file, utf8.data(), static_cast<DWORD>(required - 1), &written, nullptr);
        }
    }
    CloseHandle(file);
}

BOOL CALLBACK ResolveXInput(PINIT_ONCE, PVOID, PVOID*) {
    wchar_t systemDirectory[MAX_PATH]{};
    const UINT length = GetSystemDirectoryW(systemDirectory, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return TRUE;

    std::wstring realPath(systemDirectory, length);
    realPath += L"\\xinput1_4.dll";
    g_systemXInput = LoadLibraryExW(realPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!g_systemXInput) return TRUE;

    g_getState = reinterpret_cast<GetStateFn>(GetProcAddress(g_systemXInput, "XInputGetState"));
    g_getStateEx = reinterpret_cast<GetStateFn>(GetProcAddress(g_systemXInput, MAKEINTRESOURCEA(100)));
    g_setState = reinterpret_cast<SetStateFn>(GetProcAddress(g_systemXInput, "XInputSetState"));
    g_getCapabilities = reinterpret_cast<GetCapabilitiesFn>(GetProcAddress(g_systemXInput, "XInputGetCapabilities"));
    g_enable = reinterpret_cast<EnableFn>(GetProcAddress(g_systemXInput, "XInputEnable"));
    g_getAudioDeviceIds = reinterpret_cast<GetAudioDeviceIdsFn>(GetProcAddress(g_systemXInput, "XInputGetAudioDeviceIds"));
    g_getBatteryInformation = reinterpret_cast<GetBatteryInformationFn>(GetProcAddress(g_systemXInput, "XInputGetBatteryInformation"));
    g_getKeystroke = reinterpret_cast<GetKeystrokeFn>(GetProcAddress(g_systemXInput, "XInputGetKeystroke"));
    return TRUE;
}

void EnsureXInput() {
    InitOnceExecuteOnce(&g_resolveOnce, ResolveXInput, nullptr, nullptr);
}

DWORD WINAPI InitializeFusion(LPVOID module) {
    wchar_t gamePath[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(static_cast<HMODULE>(module), gamePath, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return 1;

    std::wstring gameDirectory(gamePath, length);
    const auto separator = gameDirectory.find_last_of(L"\\/");
    if (separator == std::wstring::npos) return 2;
    gameDirectory.resize(separator + 1);

    WriteProxyLog(gameDirectory, L"XInput proxy loaded by the game process.");
    std::wstring bootstrapPath = gameDirectory;
    bootstrapPath += L"Fusion\\FusionBootstrap.dll";

    g_fusionBootstrap = LoadLibraryW(bootstrapPath.c_str());
    if (!g_fusionBootstrap) {
        const DWORD error = GetLastError();
        wchar_t message[256]{};
        swprintf_s(message, L"Could not load FusionBootstrap.dll. Windows error: %lu", error);
        WriteProxyLog(gameDirectory, message);
        OutputDebugStringW(message);
        return 3;
    }

    WriteProxyLog(gameDirectory, L"FusionBootstrap.dll loaded successfully.");
    return 0;
}
}

extern "C" DWORD WINAPI XInputGetState(DWORD userIndex, XINPUT_STATE* state) {
    EnsureXInput(); return g_getState ? g_getState(userIndex, state) : ERROR_DEVICE_NOT_CONNECTED;
}
extern "C" DWORD WINAPI XInputGetStateEx(DWORD userIndex, XINPUT_STATE* state) {
    EnsureXInput(); return g_getStateEx ? g_getStateEx(userIndex, state) : XInputGetState(userIndex, state);
}
extern "C" DWORD WINAPI XInputSetState(DWORD userIndex, XINPUT_VIBRATION* vibration) {
    EnsureXInput(); return g_setState ? g_setState(userIndex, vibration) : ERROR_DEVICE_NOT_CONNECTED;
}
extern "C" DWORD WINAPI XInputGetCapabilities(DWORD userIndex, DWORD flags, XINPUT_CAPABILITIES* capabilities) {
    EnsureXInput(); return g_getCapabilities ? g_getCapabilities(userIndex, flags, capabilities) : ERROR_DEVICE_NOT_CONNECTED;
}
extern "C" void WINAPI XInputEnable(BOOL enable) {
    EnsureXInput(); if (g_enable) g_enable(enable);
}
extern "C" DWORD WINAPI XInputGetAudioDeviceIds(DWORD userIndex, LPWSTR renderId, UINT* renderCount, LPWSTR captureId, UINT* captureCount) {
    EnsureXInput(); return g_getAudioDeviceIds ? g_getAudioDeviceIds(userIndex, renderId, renderCount, captureId, captureCount) : ERROR_DEVICE_NOT_CONNECTED;
}
extern "C" DWORD WINAPI XInputGetBatteryInformation(DWORD userIndex, BYTE devType, XINPUT_BATTERY_INFORMATION* battery) {
    EnsureXInput(); return g_getBatteryInformation ? g_getBatteryInformation(userIndex, devType, battery) : ERROR_DEVICE_NOT_CONNECTED;
}
extern "C" DWORD WINAPI XInputGetKeystroke(DWORD userIndex, DWORD reserved, XINPUT_KEYSTROKE* keystroke) {
    EnsureXInput(); return g_getKeystroke ? g_getKeystroke(userIndex, reserved, keystroke) : ERROR_DEVICE_NOT_CONNECTED;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        HANDLE thread = CreateThread(nullptr, 0, InitializeFusion, instance, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
#endif
