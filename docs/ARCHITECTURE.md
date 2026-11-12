[ARCHITECTURE.md](https://github.com/user-attachments/files/34754470/ARCHITECTURE.md)
# Fusion Installer Architecture

## Desktop client
`Fusion.Installer` is the user-facing WPF application. It owns presentation, local preferences, account session state, game detection, and catalog browsing.

## Setup wizard
`Fusion.Setup` is a separate WPF setup application. The build script publishes the desktop client into the setup project's payload directory. Setup copies those files into the current user's LocalAppData and creates a Start Menu shortcut.

## Cloud services
The desktop client uses the Supabase public client key and the signed-in user's access token. Supabase Row Level Security is responsible for access control. Never embed the service-role key.

## Fusion Loader boundary
The loader is a separate runtime component. The installer must not claim to activate mods until a compatible loader has been built and tested. Package installation will require a documented package format, integrity checks, signature verification, compatibility checks, and safe rollback.

## Geometry Dash
The first target is the legitimate Steam edition. Game detection is best-effort and read-only. The installer does not patch or overwrite the game executable.
