[MOD_SETTINGS_V2.md](https://github.com/user-attachments/files/34994241/MOD_SETTINGS_V2.md)
# Fusion native mod settings format (version 2)

The loader reads version 1 settings for backward compatibility and writes
version 2 after a settings change.

```json
{
  "version": 2,
  "activeProfile": "default",
  "profiles": [
    {
      "id": "default",
      "name": "Default",
      "mods": {
        "example.mod": true
      }
    },
    {
      "id": "challenge",
      "name": "Challenge Run",
      "mods": {
        "example.mod": false
      }
    }
  ],
  "keybinds": [
    { "action": "open_fusion_menu", "chord": "Ctrl+Shift+F" }
  ]
}
```

Profile IDs and keybind action IDs use letters, numbers, `.`, `_`, or `-`, up
to 64 characters. Profile names are 1–80 characters. Keybind chords are
non-empty strings up to 64 characters; duplicate chords assigned to different
actions are rejected. The game adapter will define canonical key syntax and
perform actual input dispatch when implemented.

Version 1:

```json
{ "version": 1, "mods": { "example.mod": false } }
```

is loaded as the `default` profile. The in-memory migration does not rewrite the
file until the caller saves settings.
