# Geode package metadata inspection

Fusion's first Geode compatibility step is a metadata-only reader for .geode
archives. It reads the root-level mod.json entry directly from the ZIP
container and does not extract files, load DLLs, or execute package code.

The reader captures mod ID, name, version, developer, description, required
Geode loader version, declared game versions by platform, and declared mod
dependencies.

Inspection is bounded to 256 MiB per archive, 4,096 archive entries, and a
1 MiB manifest. A package must contain exactly one root-level mod.json.
Required metadata and game-version declarations are validated before a
result is returned.

**This is not a compatibility check.** Reading metadata does not establish
that a Geode mod's ABI, hooks, events, UI APIs, dependencies, or game-version
assumptions are supported by Fusion. It must not be used to load or activate
the package. Compatibility remains unsupported until a Fusion-owned host and
per-mod tests exist.
