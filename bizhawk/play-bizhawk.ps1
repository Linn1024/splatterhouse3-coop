param(
    [string]$BizHawkDirectory = (Join-Path (Split-Path (Split-Path $PSScriptRoot)) 'Bizhawk'),
    [string]$ConfigPath = (Join-Path $PSScriptRoot 'bizhawk-splatterhouse.ini')
)
$ErrorActionPreference = 'Stop'
$exe = Join-Path $BizHawkDirectory 'EmuHawk.exe'
$rom = Join-Path (Split-Path $PSScriptRoot) 'Splatterhouse 3 (USA).md'
$core = Join-Path $BizHawkDirectory 'Libretro/Cores/Splatterhouse3Coop/splatterhouse_coop_libretro.dll'
try {
    foreach ($path in @($exe, $configPath, $rom)) {
        if (-not (Test-Path -LiteralPath $path)) { throw "Missing file: $path" }
    }
    # Install a newly built core on launch. A running game keeps its mapped DLL;
    # rename that file so a fresh instance can load the update without closing
    # the existing game or discarding its progress.
    New-Item -ItemType Directory -Path (Split-Path $core) -Force | Out-Null
    foreach ($name in @('splatterhouse_engine.dll', 'splatterhouse_coop_libretro.dll')) {
        $source = Join-Path $PSScriptRoot $name
        $destination = Join-Path (Split-Path $core) $name
        if ((Test-Path -LiteralPath $source) -and
            ((-not (Test-Path -LiteralPath $destination)) -or
             ((Get-FileHash -LiteralPath $source).Hash -ne (Get-FileHash -LiteralPath $destination).Hash))) {
            try { Copy-Item -LiteralPath $source -Destination $destination -ErrorAction Stop }
            catch {
                $runningName = [IO.Path]::GetFileNameWithoutExtension($name) + '.running-' + [guid]::NewGuid().ToString('N') + '.dll'
                $runningPath = Join-Path (Split-Path $core) $runningName
                try {
                    Rename-Item -LiteralPath $destination -NewName $runningName -ErrorAction Stop
                    try { Copy-Item -LiteralPath $source -Destination $destination -ErrorAction Stop }
                    catch {
                        Rename-Item -LiteralPath $runningPath -NewName $name -ErrorAction Stop
                        throw
                    }
                    Write-Host 'Installed the updated core. Existing game windows keep their previous core; this launch opens the new one.'
                } catch { throw "Could not install $name while it is in use. Close the existing Splatterhouse window and try again. $($_.Exception.Message)" }
            }
        }
    }
    foreach ($name in @('splatterhouse_engine.dll', 'splatterhouse_coop_libretro.dll')) {
        $installed = Join-Path (Split-Path $core) $name
        if (-not (Test-Path -LiteralPath $installed)) { throw "Missing file: $installed" }
    }
    $config = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json
    $token = '*Libretro*' + ([ordered]@{ Path = $rom; CorePath = $core } | ConvertTo-Json -Compress)
    $recent = @($token) + @($config.RecentRoms.recentlist | Where-Object { $_ -ne $token })
    $config.RecentRoms.recentlist = @($recent | Select-Object -First $config.RecentRoms.MAX_RECENT_FILES)
    $config.RecentRoms.AutoLoad = $true
    $config.AutoLoadLastSaveSlot = $false
    $config.SingleInstanceMode = $false
    Copy-Item -LiteralPath $ConfigPath -Destination ($ConfigPath + '.bak') -Force
    $config | ConvertTo-Json -Depth 100 | Set-Content -LiteralPath $configPath -Encoding UTF8
    # This script is the user-invoked interactive launcher.
    Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe) -ArgumentList ('--config="' + $configPath + '"') -WindowStyle Normal
} catch {
    Add-Type -AssemblyName System.Windows.Forms
    [System.Windows.Forms.MessageBox]::Show($_.Exception.Message, 'Splatterhouse 3 co-op - launch failed') | Out-Null
    exit 1
}

