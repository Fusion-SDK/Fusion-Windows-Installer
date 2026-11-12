$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$artifacts = Join-Path $root "artifacts"
$setupOut = Join-Path $artifacts "Fusion-Setup"
$payload = Join-Path $root "src/Fusion.Setup/payload"
if (Test-Path $artifacts) { Remove-Item $artifacts -Recurse -Force }
if (Test-Path $payload) { Remove-Item $payload -Recurse -Force }
New-Item -ItemType Directory -Force -Path $setupOut, $payload, (Join-Path $payload "Fusion") | Out-Null
# Build the native Fusion Loader core.
$nativeBuild = Join-Path $artifacts "FusionLoaderBuild"

cmake -S (Join-Path $root "src/Fusion.Loader.Native") `
    -B $nativeBuild `
    -A x64

if ($LASTEXITCODE -ne 0) {
    throw "Native Fusion Loader configuration failed."
}

cmake --build $nativeBuild --config Release --parallel

if ($LASTEXITCODE -ne 0) {
    throw "Native Fusion Loader build failed."
}
Set-Content -Path (Join-Path $payload "Fusion/update-source.txt") -Value "" -NoNewline

$runtime = "win-x64"
foreach ($project in @(
  @{ Path="src/Fusion.Updater/Fusion.Updater.csproj"; Name="Fusion.Updater" },
  @{ Path="src/Fusion.Uninstaller/Fusion.Uninstaller.csproj"; Name="Fusion.Uninstaller" }
)) {
  $out = Join-Path $artifacts $project.Name
  dotnet publish (Join-Path $root $project.Path) -c Release -r $runtime --self-contained true -p:PublishSingleFile=true -o $out
  if ($LASTEXITCODE -ne 0) { throw "Publish failed: $($project.Name)" }
  Copy-Item (Join-Path $out "$($project.Name).exe") (Join-Path $payload "$($project.Name).exe") -Force
}

# The loader contract is shipped as data only; it is not a native game plugin.
dotnet publish (Join-Path $root "src/Fusion.Loader/Fusion.Loader.csproj") -c Release -o (Join-Path $payload "Fusion")
if ($LASTEXITCODE -ne 0) { throw "Publish failed: Fusion.Loader" }

$setupOutDir = Join-Path $artifacts "Fusion-SetupHost"
dotnet publish (Join-Path $root "src/Fusion.Setup/Fusion.Setup.csproj") -c Release -r $runtime --self-contained true -p:PublishSingleFile=true -o $setupOutDir
if ($LASTEXITCODE -ne 0) { throw "Publish failed: Fusion.Setup" }
Copy-Item (Join-Path $setupOutDir "Fusion.Setup.exe") (Join-Path $setupOut "Fusion-Setup.exe") -Force
Copy-Item (Join-Path $setupOutDir "payload") (Join-Path $setupOut "payload") -Recurse -Force
Write-Host "Created $setupOut/Fusion-Setup.exe"
