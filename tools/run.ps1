$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
& (Join-Path $root "tools\build.ps1")
if($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& (Join-Path $root "build\nomi_eye_simulator.exe")

