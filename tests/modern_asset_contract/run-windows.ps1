param([Parameter(Mandatory=$true)][string]$CMake,
      [Parameter(Mandatory=$true)][string]$SDL3)
$ErrorActionPreference = "Stop"
$build = Join-Path $PSScriptRoot "build"
& $CMake -S $PSScriptRoot -B $build -G "Visual Studio 17 2022" -A x64 -T ClangCL "-DSDL3_DIR=$SDL3"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $CMake --build $build --config Release --parallel 4
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$ctest = Join-Path (Split-Path $CMake) "ctest.exe"
& $ctest --test-dir $build -C Release --output-on-failure
exit $LASTEXITCODE
