param(
    [switch]$SelfContained
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$installerProject = Join-Path $root "src/Fusion.Installer/Fusion.Installer.csproj"
$setupProject = Join-Path $root "src/Fusion.Setup/Fusion.Setup.csproj"
$payload = Join-Path $root "src/Fusion.Setup/payload"
$artifacts = Join-Path $root "artifacts"

Remove-Item $artifacts -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item $payload -Recurse -Force -ErrorAction SilentlyContinue
New-Item $artifacts -ItemType Directory -Force | Out-Null
New-Item $payload -ItemType Directory -Force | Out-Null

$publishArgs = @("-c", "Release", "-r", "win-x64", "--self-contained", "false")
if ($SelfContained) {
    $publishArgs = @("-c", "Release", "-r", "win-x64", "--self-contained", "true")
}

Write-Host "Publishing Fusion Installer..."
dotnet publish $installerProject @publishArgs -o $payload
if ($LASTEXITCODE -ne 0) { throw "Fusion Installer publish failed." }

Write-Host "Publishing Fusion Setup..."
dotnet publish $setupProject @publishArgs -o (Join-Path $artifacts "Fusion-Setup")
if ($LASTEXITCODE -ne 0) { throw "Fusion Setup publish failed." }

Write-Host "Build complete: $artifacts"
