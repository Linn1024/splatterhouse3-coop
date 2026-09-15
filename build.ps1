param([string]$ToolchainBin = 'C:\TEMP2\zeroTolerance\bizhawk-integration\mingw64\bin')
$ErrorActionPreference = 'Stop'
$previousPath = $env:PATH
Push-Location $PSScriptRoot
try {
    $env:PATH = $ToolchainBin + ';' + $previousPath
    (Get-Item engine/core/m68k/m68kcpu.c).LastWriteTime = Get-Date
    & mingw32-make -C engine -f Makefile.libretro platform=win HAVE_CHD=0 HAVE_SYS_PARAM=0 GIT_VERSION= -j8 -s
    if ($LASTEXITCODE) { throw 'Core build failed' }
    & g++ launcher.cpp -o Splatterhouse3-Coop.exe -O2 -std=c++17 -static -mwindows -lwinmm -lgdi32 -luser32 -lbcrypt
    if ($LASTEXITCODE) { throw 'Launcher build failed' }
    & g++ bizhawk/splatterhouse_libretro.cpp -o bizhawk/splatterhouse_coop_libretro.dll -O2 -std=c++17 -static -shared -lbcrypt -lgdi32
    if ($LASTEXITCODE) { throw 'BizHawk adapter build failed' }
    Copy-Item -LiteralPath engine/genesis_plus_gx_libretro.dll -Destination bizhawk/splatterhouse_engine.dll
} finally {
    $env:PATH = $previousPath
    Pop-Location
}
