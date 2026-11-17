[GEODE_COMPATIBILITY.md](https://github.com/user-attachments/files/34994225/GEODE_COMPATIBILITY.md)
# Geode mod compatibility in Fusion

## Goal

Fusion should be able to host selected Geode mods without requiring users to
install or launch Geode. Fusion remains the primary loader and owns the game
integration, mod lifecycle, settings, updates, and in-game interface.

## Important compatibility boundary

A Geode mod is not just a DLL with a common entry point. It can depend on the
Geode loader ABI, Geode SDK types, hooks, event systems, UI helpers, saved-value
services, mod metadata, and other Geode mods. Loading an arbitrary Geode DLL
with `LoadLibrary` does not make those services available and can crash the
game.

Therefore Fusion cannot honestly promise universal Geode binary compatibility
without implementing and maintaining the APIs those mods actually use.

## Proposed compatibility architecture

1. **Fusion-native runtime (default):** load packages built against the Fusion
   SDK. These never require Geode.
2. **Geode package inspection:** identify a Geode package, its declared Geode
   version, platform, architecture, dependencies, and required API surface.
3. **Compatibility host:** provide a separately versioned implementation of a
   documented subset of Geode's public runtime contracts inside Fusion. This is
   a Fusion component, not a Geode installation or Geode loader dependency.
4. **Capability declarations:** each compatibility release lists supported
   Geode APIs and known limitations. A mod is rejected with a clear report if
   it requires an unsupported capability.
5. **Isolation and diagnostics:** load compatibility packages in a distinct
   lifecycle domain where possible, record API usage and failures, and prevent
   incompatible packages from blocking Fusion-native mods.
6. **Certification:** test each supported Geode mod against the exact Fusion
   build and Geometry Dash build before listing it as compatible.

## Compatibility levels

- `native`: built for Fusion; supported by Fusion's native runtime.
- `geode-subset`: Geode package uses only APIs implemented by Fusion's
  compatibility host; compatibility must be verified per release.
- `unsupported`: required APIs, architecture, dependencies, or hooks are not
  implemented.

## Non-goals

- Requiring a Geode installation.
- Claiming that every existing Geode mod works automatically.
- Reusing Geode binaries without validating their API and dependency needs.
- Shipping Geode code or branding as a substitute for implementing the
  required compatibility contracts.

## Current status

This repository currently implements a Fusion-native manifest and native DLL
loading path. It does not yet contain a Geode package parser, Geode ABI shim,
Geode hook/event compatibility services, or a compatibility test suite. The
architecture above is a design target, not a claim of existing functionality.
