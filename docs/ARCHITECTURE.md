[ARCHITECTURE.md](https://github.com/user-attachments/files/34988448/ARCHITECTURE.md)


# Fusion Windows architecture


## Overview

Fusion is a standalone Geometry Dash modding platform.

It does not require Geode and does not use a separate
desktop application as its mod-management interface.

## Components

### Fusion Setup

Responsible for installing, updating, and removing Fusion.

It is not the mod manager.

### Fusion Bootstrap

`Fusion.Launcher.exe` starts Geometry Dash suspended, loads
`Fusion/FusionBootstrap.dll` into the game process, and then resumes the game.
The bootstrap initializes the native loader from `Fusion/mods` and
`Fusion/mod-settings.json` on a worker thread.

This is the process-startup bridge only. It does not yet provide Geometry Dash
hooks, a game API adapter, or an in-game menu. Those require a version-specific
integration and testing against the selected Geometry Dash build.

### Fusion Loader

Responsible for:

- Mod discovery
- Manifest validation
- Dependency resolution
- Enabled/disabled preferences
- Native mod initialization
- Mod shutdown
- Runtime error reporting

### Fusion SDK

Provides the public API used by developers to create
Fusion-compatible mods.

## Mod Compatibility

Fusion is independent of Geode.

A Geode mod does not automatically work with Fusion.
Developers must provide a Fusion-compatible implementation.

A project may publish separate Fusion and Geode packages.

## Runtime Requirements

The native loader is initialized when Geometry Dash is started through
`Fusion.Launcher.exe`. Starting the game directly through Steam does not load
Fusion in this phase. The bootstrap currently has no game hooks or in-game UI;
mods requiring Geometry Dash APIs must wait for a version-specific adapter.

## Current package layout

- `Fusion.Setup.exe`: user-facing setup wizard; detects/selects the Steam Geometry Dash folder.
- `Fusion.Launcher.exe`: starts Geometry Dash with the native bootstrap loaded into the game process.
- `Fusion.Updater.exe`: separate updater launcher. It reads `Fusion/update-source.txt`, downloads an HTTPS release manifest, verifies SHA-256 for each file, and only accepts paths under `Fusion/`.
- `Fusion.Uninstaller.exe`: management/removal window. It reads `Fusion/installed-files.json` and removes only recorded Fusion-owned files plus the two Fusion management executables.
- `Fusion/`: Fusion-owned runtime/configuration/data directory.
- `Fusion/FusionBootstrap.dll`: native in-process bootstrap; initializes the native mod runtime.
- `Fusion.Loader.dll`: managed contract placeholder; it is not the native loader and is not loaded by Geometry Dash.

## Safety boundaries

The installer never replaces `GeometryDash.exe` or other original game files. The updater rejects paths outside `Fusion/`. The uninstaller only removes paths recorded in the Fusion-owned file manifest. Keep all update downloads on HTTPS and publish SHA-256 values generated from the exact release files.

## Loader work still required

The bootstrap now loads the native runtime into the Geometry Dash process when
started through `Fusion.Launcher.exe`. The next integration work is a tested,
version-specific Geometry Dash adapter, safe hook lifecycle, game-ready state,
and the in-game Fusion menu. The current `FusionGameApi` pointer is deliberately
null until that adapter exists. Do not claim game-aware mods or the in-game menu
are available yet.

## Release manifest format

Host `fusion-manifest.json` at a stable HTTPS URL. Example:

```json
{
  "version": "0.2.0",
  "files": [
    {
      "path": "Fusion/Fusion.Runtime.dll",
      "url": "https://github.com/OWNER/REPO/releases/download/v0.2.0/Fusion.Runtime.dll",
      "sha256": "REPLACE_WITH_64_CHARACTER_SHA256"
    }
  ]
}
```

Set the manifest URL in `Geometry Dash/Fusion/update-source.txt`. The updater intentionally updates only files inside `Fusion/`; the updater and uninstaller executables are replaced by running a new setup package.


## Native mod enable/disable preferences

The native loader supports a per-installation settings file. The application
that owns the Fusion mods directory should load it before building a load plan:

```cpp
fusion::loader::ModSettingsStore settings;
std::string settingsError;

if (!settings.Load(fusionConfigDirectory / "mod-settings.json", settingsError)) {
    // Report the settings error and do not silently overwrite the user's file.
}

fusion::loader::ModRegistry registry;
registry.Discover(modsDirectory);

fusion::loader::ModLoader loader;
loader.BuildLoadPlan(registry, settings);
```

A mod is enabled by default when it has no saved preference. To change a
preference, call `SetEnabled(modId, false)` or `SetEnabled(modId, true)`,
then call `Save(error)`.

The settings file uses versioned JSON:

```json
{
  "version": 1,
  "mods": {
    "example.mod": false
  }
}
```

Disabled mods are omitted from the load order. Mods that require a disabled
dependency are marked failed and are not loaded. This is native loader
infrastructure; the current WPF interface does not yet expose these controls,
and the loader is not yet integrated into Geometry Dash.
