param(
    [string]$Version = '0.1.0',
    [string]$SourceRef = 'HEAD',
    [string]$ToolchainBin,
    [string]$IsccPath = 'ISCC.exe'
)
$ErrorActionPreference = 'Stop'
if ($Version -notmatch '^\d+\.\d+\.\d+([.-][A-Za-z0-9.-]+)?$') { throw 'Invalid release version.' }
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    $revision = & git rev-parse --verify $SourceRef
    if ($LASTEXITCODE) { throw 'Cannot resolve the source revision.' }
    $output = Join-Path $root "dist/$Version"
    if (Test-Path $output) { throw "Output already exists: $output. Choose a new version or move the previous output." }
    $compiler = (Get-Command $IsccPath -ErrorAction Stop).Source
    New-Item -ItemType Directory -Path $output | Out-Null
    $sourceZip = Join-Path $output "Splatterhouse3-Coop-$Version-source.zip"
    & git archive --format=zip "--output=$sourceZip" $revision
    if ($LASTEXITCODE) { throw 'Source archive failed.' }
    $source = Join-Path $output 'build-source'
    Expand-Archive -LiteralPath $sourceZip -DestinationPath $source
    & (Join-Path $source 'build.ps1') -ToolchainBin $ToolchainBin
    $payload = Join-Path $output 'payload'
    New-Item -ItemType Directory -Path "$payload/engine", "$payload/bizhawk" | Out-Null
    Copy-Item -LiteralPath "$source/Splatterhouse3-Coop.exe" -Destination $payload
    Copy-Item -LiteralPath "$source/engine/genesis_plus_gx_libretro.dll", "$source/engine/LICENSE.txt" -Destination "$payload/engine"
    Copy-Item -LiteralPath "$source/bizhawk/splatterhouse_coop_libretro.dll", "$source/bizhawk/splatterhouse_engine.dll", "$source/bizhawk/install.ps1", "$source/bizhawk/README.md" -Destination "$payload/bizhawk"
    Copy-Item -LiteralPath "$source/README.md", "$source/DEVELOPING.md", "$source/RESEARCH.md", "$source/THIRD_PARTY_NOTICES.md" -Destination $payload
    Copy-Item -LiteralPath $sourceZip -Destination "$payload/source.zip"
    @"
Splatterhouse 3 Co-op $Version
Source revision: $revision
Project: https://github.com/Linn1024/splatterhouse3-coop
The complete matching source and component license notices are in source.zip.
See README.md for building and playing, and engine/LICENSE.txt for engine terms.
No game ROMs or player saves are included.
"@ | Set-Content -LiteralPath "$payload/SOURCE.txt" -Encoding UTF8
    $portable = Join-Path $output "Splatterhouse3-Coop-$Version-windows-x64-portable.zip"
    Compress-Archive -Path "$payload/*" -DestinationPath $portable
    & $compiler "/DAppVersion=$Version" "/DPayloadDir=$payload" "/DOutputDir=$output" (Join-Path $source 'installer/setup.iss')
    if ($LASTEXITCODE) { throw 'Installer compilation failed.' }
    $assets = Get-ChildItem -LiteralPath $output -File | Where-Object { $_.Extension -in '.exe', '.zip' }
    $assets | Sort-Object Name | ForEach-Object {
        $hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        "$hash  $($_.Name)"
    } | Set-Content -LiteralPath (Join-Path $output 'SHA256SUMS.txt') -Encoding ASCII
    Write-Host "Release files: $output"
} finally {
    Pop-Location
}
