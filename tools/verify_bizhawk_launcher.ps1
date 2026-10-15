$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$testRoot = Join-Path $projectRoot ('diagnostics/launcher-update-' + [guid]::NewGuid().ToString('N'))
$coreDirectory = Join-Path $testRoot 'Libretro/Cores/Splatterhouse3Coop'
New-Item -ItemType Directory -Path $coreDirectory -Force | Out-Null
Set-Content -LiteralPath (Join-Path $testRoot 'EmuHawk.exe') -Value 'test launcher placeholder'
$configPath = Join-Path $testRoot 'config.ini'
Copy-Item -LiteralPath (Join-Path $projectRoot 'bizhawk/bizhawk-splatterhouse.ini') -Destination $configPath
$enginePath = Join-Path $coreDirectory 'splatterhouse_engine.dll'
[IO.File]::WriteAllText($enginePath,'running old core')
Copy-Item -LiteralPath (Join-Path $projectRoot 'bizhawk/splatterhouse_coop_libretro.dll') -Destination $coreDirectory
# A mapped DLL can be renamed but cannot be overwritten. Keep this handle alive.
$lock = [IO.File]::Open($enginePath,[IO.FileMode]::Open,[IO.FileAccess]::Read,([IO.FileShare]::Read -bor [IO.FileShare]::Delete))
$global:appearanceTestLaunchedPath = $null
function Start-Process {
    param($FilePath,$WorkingDirectory,$ArgumentList,$WindowStyle)
    $global:appearanceTestLaunchedPath = $FilePath
}
try {
    & (Join-Path $projectRoot 'bizhawk/play-bizhawk.ps1') -BizHawkDirectory $testRoot -ConfigPath $configPath
    $expected = (Get-FileHash -LiteralPath (Join-Path $projectRoot 'bizhawk/splatterhouse_engine.dll')).Hash
    if ((Get-FileHash -LiteralPath $enginePath).Hash -ne $expected) { throw 'Updated core was not installed.' }
    $renamed = @(Get-ChildItem -LiteralPath $coreDirectory -Filter 'splatterhouse_engine.running-*.dll')
    if ($renamed.Count -ne 1 -or [IO.File]::ReadAllText($renamed[0].FullName) -ne 'running old core') { throw 'Running core was not preserved.' }
    if ($global:appearanceTestLaunchedPath -ne (Join-Path $testRoot 'EmuHawk.exe')) { throw 'Updated instance was not launched.' }
    $config = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json
    if (-not $config.RecentRoms.AutoLoad -or $config.SingleInstanceMode) { throw 'Fresh instance configuration is wrong.' }
    Write-Output 'PASS launcher replaces a locked old core by renaming it, preserves the running copy, and launches a fresh instance.'
} finally {
    $lock.Dispose()
}
