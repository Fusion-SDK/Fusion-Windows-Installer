# Fusion for Windows completion plan

This plan distinguishes repository infrastructure from features that require a
running Geometry Dash process.

## Track A — loader foundation

- [x] Native manifest discovery and validation.
- [x] Dependency-aware load planning.
- [x] Native mod lifecycle and enable/disable preferences.
- [x] Version 2 settings persistence for profiles and keybind assignments.
- [ ] Native unit tests for settings migration, profile lifecycle, and keybind
      conflict validation.
- [ ] Runtime adapters for each additional language (C#, Lua, JavaScript).
- [ ] Crash-safe mod isolation and recovery.

## Track B — Geometry Dash integration

- [ ] Confirm the exact Steam game build number and executable architecture.
- [ ] Implement and test automatic loading from the normal Steam Play action.
- [ ] Implement version-specific game API adapter and hook lifecycle.
- [ ] Add game-ready/shutdown notifications and safe hook teardown.
- [ ] Implement in-game Fusion menu and input routing.
- [ ] Connect mod enablement, profiles, keybinds, dependency details, and
      troubleshooting screens to the menu.
- [ ] Verify clean install, update, rollback, and uninstall on a clean machine.

## Track C — catalog and accounts

- [x] Initial Supabase auth and catalog read integration.
- [x] Initial profile, mod, submission, and sync tables.
- [ ] Add versioned mod releases and immutable artifact metadata.
- [ ] Add package hash/signature verification and safe download handling.
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
