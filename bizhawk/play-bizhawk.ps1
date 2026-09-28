param(
    [Parameter(Mandatory=$true)][string]$BizHawkDirectory,
    [Parameter(Mandatory=$true)][string]$ConfigPath
)
$ErrorActionPreference = 'Stop'
$exe = Join-Path $BizHawkDirectory 'EmuHawk.exe'
$rom = Join-Path (Split-Path $PSScriptRoot) 'Splatterhouse 3 (USA).md'
$core = Join-Path $BizHawkDirectory 'Libretro/Cores/Splatterhouse3Coop/splatterhouse_coop_libretro.dll'
try {
    foreach ($path in @($exe, $configPath, $rom, $core)) {
        if (-not (Test-Path -LiteralPath $path)) { throw "Missing file: $path" }
    }
    # Install a newly built core on launch. A running game keeps its loaded DLL;
    # report a useful error if it has not been closed yet.
    foreach ($name in @('splatterhouse_engine.dll', 'splatterhouse_coop_libretro.dll')) {
        $source = Join-Path $PSScriptRoot $name
        $destination = Join-Path (Split-Path $core) $name
        if ((Test-Path -LiteralPath $source) -and
            ((-not (Test-Path -LiteralPath $destination)) -or
             ((Get-FileHash -LiteralPath $source).Hash -ne (Get-FileHash -LiteralPath $destination).Hash))) {
            try { Copy-Item -LiteralPath $source -Destination $destination -ErrorAction Stop }
            catch { throw "Close the existing BizHawk game, then launch this shortcut again to install the update. $($_.Exception.Message)" }
        }
    }
    $config = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json
    $token = '*Libretro*' + ([ordered]@{ Path = $rom; CorePath = $core } | ConvertTo-Json -Compress)
    $recent = @($token) + @($config.RecentRoms.recentlist | Where-Object { $_ -ne $token })
    $config.RecentRoms.recentlist = @($recent | Select-Object -First $config.RecentRoms.MAX_RECENT_FILES)
    $config.RecentRoms.AutoLoad = $true
    $config.AutoLoadLastSaveSlot = $false
    $config.SingleInstanceMode = $false
    $config | ConvertTo-Json -Depth 100 | Set-Content -LiteralPath $configPath -Encoding UTF8
    # This script is the user-invoked interactive launcher.
    Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe) -ArgumentList ('--config="' + $configPath + '"') -WindowStyle Normal
} catch {
    Add-Type -AssemblyName System.Windows.Forms
    [System.Windows.Forms.MessageBox]::Show($_.Exception.Message, 'Splatterhouse 3 co-op - launch failed') | Out-Null
    exit 1
}

