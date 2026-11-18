# Mod Package Specification — Draft

This document defines package integrity formats 1 and 2 and the proposed signed
format 3. Format 3 is specified but not implemented; the native parser currently
rejects it until signature verification and trusted-key distribution are ready.

## Package format version 1

Existing manifests without `packageFormatVersion` are treated as format 1.
They retain the legacy behavior and do not receive file-integrity verification.
This preserves compatibility with existing local packages.

## Package format version 2

A format 2 `mod.json` adds:

```json
{
  "packageFormatVersion": 2,
  "files": [
    {
      "path": "bin/example.dll",
      "sha256": "64 lowercase hexadecimal characters"
    }
  ]
}
```

The `files` array must list every regular package file except `mod.json`,
exactly once. Paths must be normalized relative paths and cannot contain
absolute roots, `.`, or `..` segments. Symbolic links and unsupported file
types are rejected. Every listed file is hashed with SHA-256 before staging
and verified again in the staged copy. Unlisted, missing, duplicate, or
hash-mismatched files cause installation to fail before activation.

The manifest itself is not hashed because it contains the file hashes. A
format 2 package must contain at least one payload file.

SHA-256 verification detects modified or corrupted files relative to the
manifest. It does **not** prove who published the package: an attacker able to
replace both a package and its manifest can generate matching hashes. A valid
format 2 hash check must never be presented as publisher authentication.

## Future requirements

- Supported game and game-version range.
- Required Fusion Loader API version.
- Declared capabilities and permissions.
- Immutable online release metadata and trusted key policy.


## Package format version 3 — signed packages

See [PACKAGE_SIGNING_TRUST_MODEL.md](PACKAGE_SIGNING_TRUST_MODEL.md) for the
proposed detached Ed25519 signature, publisher-key trust registry, revocation,
rotation, online-install enforcement, and compatibility rollout. Format 3
must not be treated as implemented until signature verification and trust
registry distribution are complete and tested.
