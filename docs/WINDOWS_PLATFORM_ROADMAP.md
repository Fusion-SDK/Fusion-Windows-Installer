# Fusion for Windows completion plan

This plan distinguishes repository infrastructure from features that require a
running Geometry Dash process. Development can continue on the loader,
installer, catalog, and test infrastructure without Steam access; game-specific
behavior remains an explicit validation gate rather than an assumed result.

## Track A — loader foundation

- [x] Native manifest discovery and validation.
- [x] Dependency-aware load planning.
- [x] Native mod lifecycle and enable/disable preferences.
- [x] Version 2 settings persistence for profiles and keybind assignments.
- [x] Native regression tests for settings migration, profile lifecycle, and
      keybind validation (GitHub Actions run 44339463233).
- [x] Native package-manager regression tests for install, update, uninstall,
      unsafe identifiers, nested sources, and stale-operation recovery.
- [x] Native package-integrity verification for format 2 manifests (PR #11; Windows CI run 44356727748).
- [x] Document signed format 3 trust and key-rotation design (PR #12).
- [x] Reject format 3 packages until signature verification is implemented (fail-closed regression test).
- [ ] Implement Ed25519 verification and an authenticated, rollback-protected trusted-key registry.
- [ ] Runtime adapters for each additional language (C#, Lua, JavaScript).
- [ ] Crash-safe mod isolation and recovery.

## Track B — Geometry Dash integration

- [ ] Confirm the exact Steam game build number and executable architecture.
- [ ] Verify automatic loading from the normal Steam Play action in the target
      game installation.
- [ ] Implement a version-specific game API adapter and hook lifecycle.
- [ ] Add game-ready/shutdown notifications and safe hook teardown.
- [ ] Implement the in-game Fusion menu and input routing.
- [ ] Connect mod enablement, profiles, keybinds, dependency details, and
      troubleshooting screens to the menu.
- [ ] Verify clean install, update, rollback, and uninstall on a clean machine.

These checks require access to a supported Steam Geometry Dash installation.
They must remain unverified until tested against the actual game process.

## Track C — catalog and accounts

- [x] Initial Supabase auth and catalog read integration.
- [x] Initial profile, mod, submission, and sync tables.
- [ ] Define versioned mod releases and immutable artifact metadata.
- [x] Implement package hash verification for local format 2 packages.
- [ ] Implement signed format 3 release verification and safe online download handling.
- [x] Define publisher signing and trust/key-rotation policy before enforcing
      digital signatures (PR #12 design only).
- [ ] Add ratings/reviews with abuse controls and RLS policies.
- [ ] Add download/install/update records and rollback support.
- [ ] Add publisher ownership and moderation workflows.
- [ ] Add account/session refresh and sign-out handling.

## Track D — Geode compatibility without Geode installed

- [ ] Build a Geode package metadata reader.
- [ ] Inventory public Geode runtime APIs required by candidate mods.
- [ ] Implement a versioned Fusion compatibility host for a defined API subset.
- [ ] Implement compatibility capability declarations and unsupported-API
      diagnostics.
- [ ] Build a per-mod/per-game-version compatibility test suite.
- [ ] Publish a verified compatibility matrix; do not claim universal support.

## Release gate

Fusion for Windows is not feature-complete until the normal Steam Play action
loads Fusion, the in-game menu works in the target game build, native mods can
be managed from that menu, online package operations are verified, and setup,
update, rollback, and uninstall tests pass. Geode compatibility is a separate
release gate and must list its supported API subset and tested mods.
