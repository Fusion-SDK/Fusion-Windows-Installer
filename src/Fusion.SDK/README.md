# Fusion Native Mod SDK

The current Fusion mod runtime supports native Windows libraries through a
versioned C ABI. This SDK is the implemented native plugin contract; it does
not yet provide a Geometry Dash game API.

## Current capabilities and limitations

- Native Windows DLL loading and lifecycle callbacks are implemented.
- A mod exports `FusionModGetApi` and returns a stable `FusionModApi` structure.
- The host provides logging through `FusionHostApi::log`.
- `FusionHostApi::game` is currently `nullptr`. Do not dereference it or
  assume Geometry Dash hooks, rendering, input, or game state are available.
- The loader currently requires an exact `FUSION_MOD_API_VERSION` match.
- A failed `on_load` callback prevents the mod from being marked loaded.
- Loaded mods receive `on_unload` during loader shutdown.

A DLL is native code running inside the game process. It is not sandboxed.
Only load mods from sources you trust.

## Minimal C++ plugin

```cpp
#include <fusion/sdk/plugin_api.h>

namespace {

const FusionHostApi* g_host = nullptr;

int32_t OnLoad(const FusionHostApi* host)
{
    if (host == nullptr ||
        host->struct_size < sizeof(FusionHostApi) ||
        host->api_version != FUSION_MOD_API_VERSION ||
        host->log == nullptr) {
        return 0;
    }

    g_host = host;
    g_host->log(FUSION_LOG_INFO, "Example mod loaded.");
    return 1;
}

void OnUnload()
{
    if (g_host != nullptr && g_host->log != nullptr) {
        g_host->log(FUSION_LOG_INFO, "Example mod unloaded.");
    }
    g_host = nullptr;
}

const FusionModApi kModApi{
    sizeof(FusionModApi),
    FUSION_MOD_API_VERSION,
    {
        sizeof(FusionModInfo),
        "example.hello",
        "Example Mod",
        "1.0.0",
        "Example Author",
        FUSION_MOD_API_VERSION
    },
    &OnLoad,
    &OnUnload
};

} // namespace

extern "C" FUSION_MOD_EXPORT
const FusionModApi* FusionModGetApi(void)
{
    return &kModApi;
}
```

## Build setup

Include the SDK headers and build a Windows DLL with the same architecture as
the target Geometry Dash process (currently intended to be Windows x64).

If using the repository CMake target:

```cmake
add_subdirectory(path/to/Fusion/src/Fusion.SDK FusionSDK)
add_library(ExampleMod SHARED ExampleMod.cpp)
target_link_libraries(ExampleMod PRIVATE Fusion::SDK)
```

The SDK CMake target exposes headers and the C++20 compile feature. The plugin
ABI itself uses C-compatible structures and function pointers; avoid passing
C++ standard-library objects, exceptions, or ownership across the ABI boundary.

## Manifest template

Place `mod.json` at the root of the mod directory and the DLL at the relative
path named by `library`:

```json
{
  "id": "example.hello",
  "name": "Example Mod",
  "author": "Example Author",
  "version": "1.0.0",
  "requiredLoaderApi": 1,
  "library": "bin/ExampleMod.dll"
}
```

The manifest identity fields must exactly match the values returned by
`FusionModGetApi`. The library path must stay inside the mod directory.

This minimal manifest is legacy package format 1 and does not receive payload
hash verification. Format 2 local packages require a complete SHA-256 file
list. Signed format 3 packages are specified but not implemented; do not claim
publisher verification or distribute untrusted binaries as authenticated.

## Lifecycle rules

- Validate every host pointer and structure size before use.
- Keep all callbacks valid until `on_unload` completes.
- Do not throw exceptions across the C ABI.
- Do not retain host pointers after `on_unload`.
- Do not assume callbacks are invoked from a game/render thread; thread
  guarantees are not yet part of the contract.
- Keep initialization short and release mod-owned resources in `on_unload`.
- Do not access game state: the game API is not implemented yet.

The ABI and lifecycle will evolve alongside the versioned game adapter. Check
the release notes and required loader API before shipping a plugin.
