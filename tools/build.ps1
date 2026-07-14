$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$cmake = Join-Path $root ".tools\cmake\data\bin\cmake.exe"
$ninja = Join-Path $root ".tools\bin\ninja.exe"
$cc = Join-Path $root "tools\zigcc.cmd"
$cxx = Join-Path $root "tools\zigcxx.cmd"
$ar = Join-Path $root "tools\zigar.cmd"
$ranlib = Join-Path $root "tools\zigranlib.cmd"
$build = Join-Path $root "build"
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $build "zig-global-cache"
$env:ZIG_LOCAL_CACHE_DIR = Join-Path $build "zig-local-cache"

& $cmake --fresh -S $root -B $build -G Ninja `
    "-DCMAKE_MAKE_PROGRAM=$ninja" `
    "-DCMAKE_C_COMPILER=$cc" `
    "-DCMAKE_CXX_COMPILER=$cxx" `
    "-DCMAKE_AR=$ar" `
    "-DCMAKE_RANLIB=$ranlib" `
    "-DCMAKE_C_COMPILER_AR=$ar" `
    "-DCMAKE_CXX_COMPILER_AR=$ar" `
    "-DCMAKE_C_COMPILER_RANLIB=$ranlib" `
    "-DCMAKE_CXX_COMPILER_RANLIB=$ranlib" `
    -DCMAKE_BUILD_TYPE=Release
if($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $cmake --build $build --parallel
exit $LASTEXITCODE
