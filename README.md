# Fusion — Windows Installer

**The official Windows installer for Fusion, a Geometry Dash modding platform.**

Fusion aims to make discovering, installing, and managing Geometry Dash mods easier.

It is designed to run directly alongside [Geometry Dash](https://store.steampowered.com/app/322170/Geometry_Dash/).

---

## Architecture

- **Fusion Bootstrap** - Starts Fusion's native integration with Geometry Dash.
- **Fusion Loader** - Discovers, validates, and loads Fusion mods.
- **Fusion SDK** - Provides APIs for developers building Fusion mods.
- **Fusion Mod Catalog** - Provides mod discovery and distribution.
- **Fusion Setup** - Installs and updates Fusion components.

Fusion is not a seperate desktop mod manager. Its mod experience is intended to run inside [GeometryDash](https://store.steampowered.com/app/322170/Geometry_Dash/).

## Compatibility

Fusion mods use the Fusion SDK and native package format.

Geode mods are not automatically compatible with Fusion.
A mod developer must provide a native Fusion implementation
or a separately maintained Fusion-compatible package.

## Requirements

- Windows
- A supported Geometry Dash installation
- Fusion's own native integration

Geode is not required.


## About Fusion

Fusion is an independent Geometry Dash modding ecosystem designed to bring mod discovery, installation, and management together in one place.

The Windows Installer is the entry point to the Fusion experience.

### Planned Features

- **Fusion Setup** — A dedicated Windows installation experience.
- **Game Detection** — Locate supported Geometry Dash installations.
- **Mod Discovery** — Browse community-created mods.
- **Mod Management** — Manage installed mods from one application.
- **Fusion SDK Integration** — Support for developers building Fusion-compatible mods.
- **Account Integration** — Connect with the Fusion ecosystem.

*Features are subject to development and may not yet be available.*

---

## Platform Support

| Platform | Support |
|---|---|
| Windows Steam Geometry Dash | Planned |

---

## Project Status

<badge>In Development</badge>

Fusion Circle is actively being developed. Features, interfaces, and compatibility may change as development progresses.

This repository focuses on the Windows installer and its associated application.

---

## For Developers

We welcome developers interested in contributing to Fusion.

You can help with:
- Windows application development
- Installer development
- UI/UX improvements
- Testing and bug reporting
- Documentation

Please read [CONTRIBUTING.md](CONTRIBUTING.md) before submitting contributions.

---

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
