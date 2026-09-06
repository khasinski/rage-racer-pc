param([Parameter(Mandatory=$true)][string]$CMake)
$ErrorActionPreference = "Stop"
$source = $PSScriptRoot
$build = Join-Path $source "build"
& $CMake -S $source -B $build -G "Visual Studio 17 2022" -A x64 -T ClangCL
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $CMake --build $build --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$ctest = Join-Path (Split-Path $CMake) "ctest.exe"
& $ctest --test-dir $build -C Release --output-on-failure
exit $LASTEXITCODE
