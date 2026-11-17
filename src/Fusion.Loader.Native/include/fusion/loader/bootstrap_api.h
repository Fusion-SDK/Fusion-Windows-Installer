
#pragma once

#include <cstddef>

#if defined(_WIN32)
    #define FUSION_BOOTSTRAP_EXPORT __declspec(dllexport)
#else
    #define FUSION_BOOTSTRAP_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Initializes Fusion using the supplied directories.
// Returns 1 on success and 0 on failure.
FUSION_BOOTSTRAP_EXPORT int FusionInitialize(
    const wchar_t* modsDirectory,
    const wchar_t* settingsFile
);

// Shuts down Fusion and unloads initialized mods.
FUSION_BOOTSTRAP_EXPORT void FusionShutdown();

// Returns the current LoaderState as an integer.
FUSION_BOOTSTRAP_EXPORT int FusionGetState();

// Copies the last error into the supplied buffer.
// Returns the number of characters required, excluding the null terminator.
FUSION_BOOTSTRAP_EXPORT std::size_t FusionGetLastError(
    wchar_t* buffer,
    std::size_t bufferSize
);

#ifdef __cplusplus
}
#endif
