[ARCHITECTURE.md](https://github.com/user-attachments/files/34984182/ARCHITECTURE.md)

# Fusion Windows architecture

## Current package layout

- `Fusion.Setup.exe`: user-facing setup wizard; detects/selects the Steam Geometry Dash folder.
- `Fusion.Updater.exe`: separate updater launcher. It reads `Fusion/update-source.txt`, downloads an HTTPS release manifest, verifies SHA-256 for each file, and only accepts paths under `Fusion/`.
- `Fusion.Uninstaller.exe`: management/removal window. It reads `Fusion/installed-files.json` and removes only recorded Fusion-owned files plus the two Fusion management executables.
- `Fusion/`: Fusion-owned runtime/configuration/data directory.
- `Fusion.Loader.dll`: currently a contract placeholder only. It is NOT a native Geometry Dash plugin and is NOT loaded by Geometry Dash.

## Safety boundaries

The installer never replaces `GeometryDash.exe` or other original game files. The updater rejects paths outside `Fusion/`. The uninstaller only removes paths recorded in the Fusion-owned file manifest. Keep all update downloads on HTTPS and publish SHA-256 values generated from the exact release files.

## Loader work still required

A real Geometry Dash loader needs a supported native integration layer, game-version compatibility checks, crash-safe startup, and testing against legally obtained game installations. The current `Fusion.Loader` project only defines a boundary for that work. Do not advertise mod loading or activation as available until a real loader is implemented and tested.

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
