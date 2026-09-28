param(
    [Parameter(Mandatory=$true)][string]$SetupPath,
    [Parameter(Mandatory=$true)][string]$RomPath
)
$ErrorActionPreference = 'Stop'
$setup = (Resolve-Path -LiteralPath $SetupPath).Path
$rom = (Resolve-Path -LiteralPath $RomPath).Path
$expectedRom = '8c7737912cf948a606a683e32f4b6a0a4303215cdc99b39fbc5976c196c45710'
if ((Get-FileHash -LiteralPath $rom).Hash -ne $expectedRom) { throw 'Supply the supported ROM for this test.' }
$uninstallKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\{58E8ED9F-5FEA-4725-8FAC-9DFFAC0DB303}_is1'
if (Test-Path $uninstallKey) { throw 'An installed copy already exists. Run this test in a clean Windows account.' }
$root = Join-Path (Split-Path $PSScriptRoot -Parent) ('dist/installer-test-' + [guid]::NewGuid().ToString('N'))
$app = Join-Path $root 'Game folder with spaces'
$group = 'Splatterhouse 3 Co-op'
New-Item -ItemType Directory -Path $root | Out-Null

function Invoke-Setup([string]$RomArgument, [string]$LogName) {
    $arguments = @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/TASKS=',
        ('/DIR="' + $app + '"'), ('/GROUP="' + $group + '"'), ('/LOG="' + (Join-Path $root $LogName) + '"'))
    if ($RomArgument) { $arguments += '/ROMFILE="' + $RomArgument + '"' }
    $process = Start-Process -FilePath $setup -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru
    return $process.ExitCode
}
function Uninstall-Test {
    $process = Start-Process -FilePath (Join-Path $app 'unins000.exe') -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART' -WindowStyle Hidden -Wait -PassThru
    if ($process.ExitCode -ne 0) { throw "Uninstall failed: $($process.ExitCode)" }
}
function Boot-Test([int]$ExpectedExit) {
    $process = Start-Process -FilePath (Join-Path $app 'Splatterhouse3-Coop.exe') -ArgumentList '--test','120' -WindowStyle Hidden -Wait -PassThru
    if ($process.ExitCode -ne $ExpectedExit) { throw "Boot returned $($process.ExitCode), expected $ExpectedExit." }
}

try {
if ((Invoke-Setup '' 'no-rom.log') -ne 0) { throw 'Installation without a ROM failed.' }
if (Test-Path (Join-Path $app 'Splatterhouse 3 (USA).md')) { throw 'Unexpected ROM in installer payload.' }
Boot-Test 1
$shortcut = Join-Path ([Environment]::GetFolderPath('Programs')) "$group/Splatterhouse 3 Co-op.lnk"
if (-not (Test-Path $shortcut)) { throw 'Start-menu shortcut missing.' }
$shell = New-Object -ComObject WScript.Shell
if ($shell.CreateShortcut($shortcut).TargetPath -ne (Join-Path $app 'Splatterhouse3-Coop.exe')) { throw 'Incorrect shortcut target.' }
Uninstall-Test
Write-Host 'PASS: installation without ROM, shortcut, missing-ROM error, uninstall'

$invalid = Join-Path $root 'invalid.bin'
Set-Content -LiteralPath $invalid -Value 'not a game ROM'
if ((Invoke-Setup $invalid 'invalid-rom.log') -eq 0) { throw 'Invalid ROM was accepted.' }
if (Test-Path (Join-Path $app 'Splatterhouse3-Coop.exe')) { throw 'Invalid ROM test installed files.' }
Write-Host 'PASS: invalid ROM rejected before installation'

if ((Invoke-Setup $rom 'valid-rom.log') -ne 0) { throw 'Installation with a valid ROM failed.' }
$importedRom = Join-Path $app 'Splatterhouse 3 (USA).md'
if ((Get-FileHash -LiteralPath $importedRom).Hash -ne $expectedRom) { throw 'Imported ROM mismatch.' }
foreach ($file in @('source.zip','SOURCE.txt','engine/LICENSE.txt','bizhawk/splatterhouse_coop_libretro.dll')) {
    if (-not (Test-Path (Join-Path $app $file))) { throw "Missing installed file: $file" }
}
Boot-Test 0
$save = Join-Path $app 'coop-save.state'
Set-Content -LiteralPath $save -Value 'installer preservation test - not a gameplay save'
$saveHash = (Get-FileHash -LiteralPath $save).Hash
if ((Invoke-Setup '' 'upgrade.log') -ne 0) { throw 'Reinstall failed.' }
if ((Get-FileHash -LiteralPath $save).Hash -ne $saveHash) { throw 'Reinstall changed save.' }
if ((Get-FileHash -LiteralPath $importedRom).Hash -ne $expectedRom) { throw 'Reinstall changed ROM.' }
Boot-Test 0
Uninstall-Test
if (Test-Path (Join-Path $app 'Splatterhouse3-Coop.exe')) { throw 'Uninstall left the executable.' }
if (Test-Path $shortcut) { throw 'Uninstall left the shortcut.' }
if ((Get-FileHash -LiteralPath $save).Hash -ne $saveHash) { throw 'Uninstall changed save.' }
if ((Get-FileHash -LiteralPath $importedRom).Hash -ne $expectedRom) { throw 'Uninstall changed ROM.' }
if ((Get-FileHash -LiteralPath $rom).Hash -ne $expectedRom) { throw 'Original ROM changed.' }
Write-Host "PASS: ROM import, boot, reinstall, uninstall and save preservation. Logs: $root"
} finally {
    if (Test-Path (Join-Path $app 'unins000.exe')) { Uninstall-Test }
}
