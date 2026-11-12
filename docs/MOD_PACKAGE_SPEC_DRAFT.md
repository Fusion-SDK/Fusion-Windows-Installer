# Mod Package Specification — Draft

This document is a planning placeholder, not an enabled installation format.

A future package format should define:
- Unique package identifier
- Human-readable name and description
- Semantic version
- Author/publisher identity
- Supported game and game-version range
- Required Fusion Loader API version
- File manifest with SHA-256 hashes
- Declared permissions/capabilities
- Digital signature and trusted publisher identity

A future installer must reject unsigned, malformed, incompatible, or hash-mismatched packages. It should stage changes, verify them, and support rollback. Do not activate packages until the specification and loader implementation have been reviewed and tested.
