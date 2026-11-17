#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <filesystem>
#include <cwchar>
#include <iostream>
#include <string>
#include <vector>

namespace {
    std::wstring Quote(const std::wstring& value)
    {
        std::wstring result = L"\"";
        unsigned int backslashes = 0;
        for (wchar_t ch : value) {
            if (ch == L'\\') {
                ++backslashes;
            } else if (ch == L'\"') {
                result.append(backslashes * 2 + 1, L'\\');
                result.push_back(L'\"');
                backslashes = 0;
            } else {
                result.append(backslashes, L'\\');
                backslashes = 0;
                result.push_back(ch);
            }
        }
        result.append(backslashes * 2, L'\\');
        result.push_back(L'\"');
        return result;
    }

    bool InjectBootstrap(HANDLE process, const std::filesystem::path& dll)
    {
        const std::wstring path = dll.wstring();
        const SIZE_T bytes = (path.size() + 1) * sizeof(wchar_t);
        void* remotePath = VirtualAllocEx(process, nullptr, bytes,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remotePath) {
            std::wcerr << L"Fusion could not allocate memory in Geometry Dash (" << GetLastError() << L").\\n";
            return false;
        }

        SIZE_T written = 0;
        if (!WriteProcessMemory(process, remotePath, path.c_str(), bytes, &written) || written != bytes) {
            std::wcerr << L"Fusion could not prepare the bootstrap path (" << GetLastError() << L").\\n";
            VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
            return false;
        }

        HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
        auto loadLibrary = kernel ? GetProcAddress(kernel, "LoadLibraryW") : nullptr;
        if (!loadLibrary) {
            std::wcerr << L"Fusion could not locate the Windows library loader.\\n";
            VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
            return false;
        }

        HANDLE thread = CreateRemoteThread(process, nullptr, 0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibrary), remotePath, 0, nullptr);
        if (!thread) {
            std::wcerr << L"Fusion could not start its bootstrap in Geometry Dash (" << GetLastError() << L").\\n";
            VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
            return false;
        }

        const DWORD wait = WaitForSingleObject(thread, 30000);
        DWORD moduleResult = 0;
        const bool succeeded = wait == WAIT_OBJECT_0 && GetExitCodeThread(thread, &moduleResult) && moduleResult != 0;
        if (!succeeded)
            std::wcerr << L"Fusion bootstrap did not load successfully.\\n";

        CloseHandle(thread);
        VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
        return succeeded;
    }
    bool WaitForBootstrap(DWORD processId)
    {
        wchar_t readyName[96]{};
        wchar_t failedName[112]{};
        swprintf_s(readyName, _countof(readyName),
            L"Local\\FusionBootstrapReady_%lu", processId);
        swprintf_s(failedName, _countof(failedName),
            L"Local\\FusionBootstrapFailed_%lu", processId);

        HANDLE events[2]{};
        for (int attempt = 0; attempt < 100 && (!events[0] || !events[1]); ++attempt) {
            if (!events[0]) events[0] = OpenEventW(SYNCHRONIZE, FALSE, readyName);
            if (!events[1]) events[1] = OpenEventW(SYNCHRONIZE, FALSE, failedName);
            if (!events[0] || !events[1]) Sleep(100);
        }
        if (!events[0] || !events[1]) {
            std::wcerr << L"Fusion bootstrap did not report startup status.\n";
            if (events[0]) CloseHandle(events[0]);
            if (events[1]) CloseHandle(events[1]);
            return false;
        }

        const DWORD result = WaitForMultipleObjects(2, events, FALSE, 60000);
        CloseHandle(events[0]);
        CloseHandle(events[1]);
        if (result == WAIT_OBJECT_0) return true;
        if (result == WAIT_OBJECT_0 + 1)
            std::wcerr << L"Fusion bootstrap initialization failed.\n";
        else
            std::wcerr << L"Fusion bootstrap initialization timed out.\n";
        return false;
    }

}

int wmain(int argc, wchar_t* argv[])
{
    wchar_t launcherPath[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, launcherPath, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        std::wcerr << L"Could not determine the Fusion launcher location.\\n";
        return 1;
    }

    const auto gameDirectory = std::filesystem::path(launcherPath).parent_path();
    const auto gameExecutable = gameDirectory / L"GeometryDash.exe";
    const auto bootstrap = gameDirectory / L"Fusion" / L"FusionBootstrap.dll";
    if (!std::filesystem::is_regular_file(gameExecutable) || !std::filesystem::is_regular_file(bootstrap)) {
        std::wcerr << L"GeometryDash.exe or Fusion\\FusionBootstrap.dll is missing. Run Fusion Setup again.\\n";
        return 2;
    }

    std::wstring commandLine = Quote(gameExecutable.wstring());
    for (int i = 1; i < argc; ++i) {
        commandLine.push_back(L' ');
        commandLine += Quote(argv[i]);
    }
    std::vector<wchar_t> mutableCommand(commandLine.begin(), commandLine.end());
    mutableCommand.push_back(L'\0');

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(gameExecutable.c_str(), mutableCommand.data(), nullptr, nullptr,
            FALSE, CREATE_SUSPENDED, nullptr, gameDirectory.c_str(), &startup, &process)) {
        std::wcerr << L"Could not start Geometry Dash (" << GetLastError() << L").\\n";
        return 3;
    }

    if (!InjectBootstrap(process.hProcess, bootstrap) ||
        !WaitForBootstrap(process.dwProcessId)) {
        TerminateProcess(process.hProcess, 1);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return 4;
    }

    ResumeThread(process.hThread);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return 0;
}
