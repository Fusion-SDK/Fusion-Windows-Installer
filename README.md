[README.md](https://github.com/user-attachments/files/34753833/README.md)
# Fusion Installer for Windows

A native Windows desktop starter for the Fusion modding ecosystem.

## Technology

- C# and .NET 10
- WPF / XAML
- Supabase Auth and PostgREST through `HttpClient`
- PowerShell build and packaging scripts
- GitHub Actions Windows CI

## Projects

- `src/Fusion.Installer`: the desktop mod manager.
- `src/Fusion.Setup`: a separate branded setup wizard that installs the published desktop app.
- `tests/Fusion.Installer.Tests`: lightweight .NET tests.

## Important implementation status

This repository is a functional application foundation, not a finished Geometry Dash mod loader.

- Supabase login and registration use the Supabase Auth REST API.
- The mod browser reads approved entries from the Supabase `mods` table.
- Steam Geometry Dash detection checks common Steam library locations.
- Update checking is a metadata check against a configurable JSON endpoint.
- Mod installation is intentionally not enabled yet. A real package format, trusted signing/verification, and a tested Fusion Loader runtime must exist before packages can safely be activated.
- Fusion Setup installs the desktop application files produced by the build script. It does not modify Geometry Dash or redistribute game files.

Use a legitimate Steam installation of Geometry Dash. Do not distribute Geometry Dash game files or proprietary assets.

## Requirements

- Windows 10 or 11, 64-bit
- .NET 10 SDK
- PowerShell 7 recommended (Windows PowerShell 5.1 may also work)
- A Supabase project for online account/catalog features

## Configure Supabase

1. Create a Supabase project.
2. Copy `.env.example` to `.env.local`.
3. Fill in the project URL and **publishable/anon key**.
4. Run `supabase/migrations/0001_initial.sql` in the Supabase SQL Editor.
5. Never put a Supabase service-role key in this desktop application.

The app reads configuration from environment variables:
- `FUSION_SUPABASE_URL`
- `FUSION_SUPABASE_ANON_KEY`
- `FUSION_UPDATES_URL`

For local development, the launch profile in `Properties/launchSettings.json` reads values from environment variables. Set them in your terminal before running, or configure them in your IDE. `.env.local` is a reference file; .NET does not automatically load it.

Example PowerShell session:

```powershell
$env:FUSION_SUPABASE_URL = "https://YOUR_PROJECT.supabase.co"
$env:FUSION_SUPABASE_ANON_KEY = "YOUR_PUBLISHABLE_OR_ANON_KEY"
dotnet run --project src/Fusion.Installer
```

## Build and run

```powershell
dotnet restore Fusion.sln
dotnet build Fusion.sln -c Release
dotnet run --project src/Fusion.Installer
```

## Build a standalone installer package

Run from the repository root:

```powershell
.\scripts\build-windows.ps1
```

The script publishes Fusion Installer, stages it as the Fusion Setup payload, then publishes Fusion Setup. Output is placed under `artifacts/`.

To create a self-contained x64 build:

```powershell
.\scripts\build-windows.ps1 -SelfContained
```

The setup wizard installs to the current user's LocalAppData by default and creates a Start Menu shortcut. It does not require administrator access.

## Tests

```powershell
dotnet test Fusion.sln -c Release
```

## Repository structure

```text
Fusion-Windows/
├── Fusion.sln
├── src/
│   ├── Fusion.Installer/
│   └── Fusion.Setup/
├── tests/
├── supabase/
├── scripts/
└── .github/workflows/
```

## Security notes

- Keep `.env.local`, tokens, and credentials out of Git.
- Only use the public Supabase client key in a desktop app; enforce access with Row Level Security.
- Treat mod packages as untrusted input.
- Do not enable package installation until signature verification and loader compatibility checks are implemented.
- Do not overwrite or patch `GeometryDash.exe`.

## License

MIT. See `LICENSE`.
