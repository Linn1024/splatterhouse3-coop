param([Parameter(Mandatory=$true)][string]$BizHawkDirectory)
$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath (Join-Path $BizHawkDirectory 'EmuHawk.exe'))) {
    throw 'BizHawkDirectory must contain EmuHawk.exe.'
}
$destination = Join-Path $BizHawkDirectory 'Libretro/Cores/Splatterhouse3Coop'
New-Item -ItemType Directory -Path $destination -Force | Out-Null
foreach ($name in @('splatterhouse_coop_libretro.dll','splatterhouse_engine.dll')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $name) -Destination (Join-Path $destination $name)
}
Write-Host "Installed in $destination. Load this core in BizHawk."
