[ARCHITECTURE.md](https://github.com/user-attachments/files/34994196/ARCHITECTURE.md)
# Fusion for Windows architecture

## Product requirements

Fusion is an independent Geometry Dash modding platform for Steam on Windows.
It must not require Geode, and its mod-management experience belongs inside
Geometry Dash rather than in a separate desktop manager. The Windows setup,
updater, and uninstaller remain companion maintenance tools.

The target feature set includes an in-game mod browser and manager, profiles,
keybinds, dependency management, troubleshooting, online accounts, catalog
ratings, downloads, and updates. Runtime support is intended to cover the
languages and plugin systems used by Fusion, with native C++ as the current
implemented path. Additional language runtimes must be added and tested
individually; a manifest label alone does not provide a runtime.

## Existing components

- **Fusion.Setup:** installs Fusion-owned files into the selected Geometry Dash
  directory without replacing original game files.
- **Fusion.Updater:** verifies HTTPS release manifests and SHA-256 file hashes.
- **Fusion.Uninstaller:** removes only files recorded in the Fusion-owned
  installation manifest.
- **Fusion Loader Native:** discovers manifests, validates dependencies,
  resolves load order, reads settings, and loads native mod libraries.
- **Fusion SDK:** defines the current native C plugin contract.
- **Fusion Installer:** contains account and catalog UI/service code. It is a
  maintenance/catalog companion, not the in-game mod manager.
- **Supabase:** provides the initial account/catalog schema and auth integration.

## Startup integration requirement

The Windows build includes an `XInput1_4.dll` proxy. Geometry Dash's process
loads the proxy through its normal XInput import, and the proxy forwards XInput
exports to the real System32 DLL while loading `Fusion\FusionBootstrap.dll`
in-process. This avoids a separate launcher and does not patch the game EXE.
The proxy path must be validated against each target game build and may conflict
with another mod loader that already owns the same proxy filename.
The XInput proxy path is implemented in the Windows build, but it must be
validated against the exact Steam game build and alongside other proxy-based
mods. The installer backs up an existing `XInput1_4.dll` and the uninstaller
restores it. A proxy filename collision with another loader remains a known
compatibility concern; do not claim universal coexistence until tested.

## Game integration and menu

The native bootstrap currently initializes the loader but does not expose a
Geometry Dash game API, hook lifecycle, game-ready signal, rendering/input
integration, or in-game menu. Those require a version-specific adapter. The
menu must be rendered and receive input through that adapter, not through a
separate desktop application.

## Native settings, profiles, and keybind assignments

The native settings store now reads legacy version 1 settings and writes
version 2 settings. Version 2 stores named profiles, the active profile,
per-profile mod enablement, and keybind assignments. This is persistence and
configuration infrastructure only: keybind dispatch and profile switching
from the in-game menu require the not-yet-implemented game input/UI adapter.

## Geode compatibility

Fusion-native mods remain the default and do not require Geode. Supporting
Geode mods without installing Geode requires a Fusion-owned compatibility host
that implements the Geode API subset required by each supported mod. Arbitrary
Geode DLLs cannot be assumed compatible merely because Fusion can load a DLL.
See [GEODE_COMPATIBILITY.md](GEODE_COMPATIBILITY.md). The compatibility host
and certification suite are not implemented yet.

## Runtime language support

The current native loader loads native Windows libraries through the Fusion C
plugin contract. C++ is the implemented mod runtime. C#/.NET, Lua, and
JavaScript require explicit runtime hosts, lifecycle adapters, package
validation, dependency handling, and versioned SDK contracts before they can
be advertised as supported.

## Online catalog

The current Supabase migration contains profiles, approved mods, submissions,
and user sync data. The current client supports registration, sign-in, and
reading approved catalog entries. Secure package delivery, version history,
ratings/reviews, download records, moderation workflows, and client-side
installation/update integration remain incomplete. Privileged Supabase
service-role keys must never be embedded in the Windows client.

## Release safety

- Never overwrite `GeometryDash.exe` or other game-owned files.
- Restrict updater writes to the Fusion-owned directory.
- Verify downloaded files against release-manifest SHA-256 values.
- Preserve existing user settings and mods during upgrades.
- Only remove Fusion-owned files recorded by the installer.
- Keep compatibility claims tied to tested game, loader, and mod versions.
