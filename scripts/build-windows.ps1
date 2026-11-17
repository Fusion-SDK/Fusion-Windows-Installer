$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$artifacts = Join-Path $root "artifacts"
$packageRoot = Join-Path $artifacts "Fusion-Package"
$payload = Join-Path $packageRoot "payload"
$fusionPayload = Join-Path $payload "Fusion"
$runtime = "win-x64"

if (Test-Path $artifacts) {
    Remove-Item $artifacts -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $packageRoot, $payload, $fusionPayload | Out-Null

function Publish-FusionProject {
    param(
        [Parameter(Mandatory = $true)][string]$Project,
        [Parameter(Mandatory = $true)][string]$Output,
        [string[]]$AdditionalArguments = @()
    )

    $projectPath = Join-Path $root $Project
    dotnet publish $projectPath --configuration Release @AdditionalArguments --output $Output

    if ($LASTEXITCODE -ne 0) {
        throw "Publish failed: $Project"
    }
}

# Build the native loader core, in-process bootstrap DLL, and the XInput proxy
# that loads Fusion automatically when Geometry Dash starts through Steam.
$nativeBuild = Join-Path $artifacts "FusionLoaderBuild"
cmake -S (Join-Path $root "src/Fusion.Loader.Native") -B $nativeBuild -A x64
if ($LASTEXITCODE -ne 0) {
    throw "Native Fusion Loader configuration failed."
}

cmake --build $nativeBuild --config Release --parallel
if ($LASTEXITCODE -ne 0) {
    throw "Native Fusion Loader build failed."
}

$nativeRelease = Join-Path $nativeBuild "Release"
$bootstrapDll = Join-Path $nativeRelease "FusionBootstrap.dll"
$xinputProxy = Join-Path $nativeRelease "XInput1_4.dll"
if (!(Test-Path $bootstrapDll) -or !(Test-Path $xinputProxy)) {
    throw "Native bootstrap or XInput proxy output is missing."
}
Copy-Item $bootstrapDll (Join-Path $fusionPayload "FusionBootstrap.dll") -Force
Copy-Item $xinputProxy (Join-Path $payload "XInput1_4.dll") -Force

# Main Fusion desktop application and managed loader contract.
Publish-FusionProject `
    -Project "src/Fusion.Installer/Fusion.Installer.csproj" `
    -Output $fusionPayload `
    -AdditionalArguments @("--runtime", $runtime, "--self-contained", "true", "-p:PublishSingleFile=true")

Publish-FusionProject `
    -Project "src/Fusion.Loader/Fusion.Loader.csproj" `
    -Output $fusionPayload

# Supporting tools are installed beside the Fusion directory.
foreach ($project in @(
    @{ Path = "src/Fusion.Updater/Fusion.Updater.csproj"; Name = "Fusion.Updater" },
    @{ Path = "src/Fusion.Uninstaller/Fusion.Uninstaller.csproj"; Name = "Fusion.Uninstaller" }
)) {
    $output = Join-Path $artifacts $project.Name
    Publish-FusionProject `
        -Project $project.Path `
        -Output $output `
        -AdditionalArguments @("--runtime", $runtime, "--self-contained", "true", "-p:PublishSingleFile=true")

    $executable = Join-Path $output "$($project.Name).exe"
    if (!(Test-Path $executable)) {
        throw "Expected executable was not produced: $executable"
    }

    Copy-Item $executable (Join-Path $payload "$($project.Name).exe") -Force
}

# Publish the setup host separately, then place it beside its payload so
# AppContext.BaseDirectory/payload resolves correctly when NSIS extracts it.
$setupOutput = Join-Path $artifacts "Fusion-SetupHost"
Publish-FusionProject `
    -Project "src/Fusion.Setup/Fusion.Setup.csproj" `
    -Output $setupOutput `
    -AdditionalArguments @("--runtime", $runtime, "--self-contained", "true", "-p:PublishSingleFile=true")

$setupExecutable = Join-Path $setupOutput "Fusion.Setup.exe"
if (!(Test-Path $setupExecutable)) {
    throw "Fusion.Setup.exe was not produced."
}
Copy-Item $setupExecutable (Join-Path $packageRoot "Fusion.Setup.exe") -Force

# Validate the exact layout expected by Fusion.Setup before packaging.
$requiredFiles = @(
    (Join-Path $packageRoot "Fusion.Setup.exe"),
    (Join-Path $payload "XInput1_4.dll"),
    (Join-Path $fusionPayload "FusionBootstrap.dll"),
    (Join-Path $payload "Fusion.Updater.exe"),
    (Join-Path $payload "Fusion.Uninstaller.exe"),
    (Join-Path $fusionPayload "Fusion.Installer.exe"),
    (Join-Path $fusionPayload "Fusion.Loader.dll")
)

$missingFiles = $requiredFiles | Where-Object { !(Test-Path $_) }
if ($missingFiles.Count -gt 0) {
    throw "Installer package is incomplete. Missing files:`n$($missingFiles -join "`n")"
}

Write-Host "Fusion installer package assembled successfully: $packageRoot"
Get-ChildItem $packageRoot -Recurse -File | ForEach-Object {
    Write-Host (Resolve-Path -Relative $_.FullName)
}
