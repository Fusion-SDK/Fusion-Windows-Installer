[README.md](https://github.com/user-attachments/files/34994376/README.md)
# Fusion — Windows Installer

**The official Windows installer for Fusion, a Geometry Dash modding platform.**

Fusion aims to make discovering, installing, and managing Geometry Dash mods easier.

It is designed to run directly alongside [Geometry Dash](https://store.steampowered.com/app/322170/Geometry_Dash/).

---

## Mod developer SDK

See [the native mod SDK guide](src/Fusion.SDK/README.md) for the current C++ plugin ABI, lifecycle, and limitations.
## Backend setup

See [Supabase setup and deployment](docs/SUPABASE_SETUP.md) before configuring the account/catalog features.

## Architecture

- **Fusion Bootstrap and Loader** — native startup and mod lifecycle infrastructure.
- **Fusion SDK** — the current native C plugin contract.
- **Fusion Setup, Updater, and Uninstaller** — install and maintain Fusion-owned files.
- **Fusion Catalog** — Supabase-backed account and approved-mod catalog foundation.
- **In-game integration** — planned Geometry Dash adapter and Fusion menu; not yet implemented.

Fusion is not intended to require a separate desktop mod manager. The setup and
maintenance tools are separate Windows utilities, while mod management is intended
to happen inside Geometry Dash.

## Compatibility

Fusion-native mods do not require Geode. Fusion's current native runtime does not
yet provide general Geode binary compatibility. Supporting selected Geode mods
without installing Geode requires a Fusion-owned compatibility host for the
Geode APIs those mods use. Compatibility must be verified per mod and game build;
it cannot be promised for every Geode mod automatically. See
[Geode compatibility design](docs/GEODE_COMPATIBILITY.md).

## Current status

The repository includes native mod discovery, dependency-aware load planning,
mod lifecycle support, and settings persistence. The settings store now supports
named profiles and keybind assignments, including migration from the earlier
settings format. These settings are not yet connected to an in-game menu.

The Windows package now includes an XInput1_4 proxy intended to load the native
bootstrap in-process when Geometry Dash starts normally through Steam. It
forwards XInput calls to the system DLL. This startup integration still needs
validation on the target Steam game build and alongside other proxy-based mods.
The Geometry Dash game API adapter, in-game Fusion menu, additional language
runtimes, complete online package delivery/ratings, and Geode compatibility host
remain development work. See [the Windows completion plan](docs/WINDOWS_PLATFORM_ROADMAP.md).

## Requirements

- Windows x64
- Steam Geometry Dash (the exact supported game build will be recorded per release)
- Geode is not required for Fusion-native mods

## Bug Reports & Suggestions

Found a problem or have an idea?

- [Report a Bug](../../issues/new?template=bug_report.md)
- [Request a Feature](../../issues/new?template=feature_request.md)

---

## Related Projects

- **Fusion SDK** — Tools and resources for Fusion-compatible mod development.
- **Fusion** — The broader Geometry Dash modding ecosystem.

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

<p align="center">
  Fusion
  <br>
  Built for the Geometry Dash modding community.
</p>
