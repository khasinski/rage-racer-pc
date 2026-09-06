param(
    [string]$CMake = 'cmake',
    [string]$BuildDirectory = (Join-Path $PSScriptRoot '../../build/mod-contract-windows')
)
$ErrorActionPreference = 'Stop'
& $CMake -S $PSScriptRoot -B $BuildDirectory -G 'Visual Studio 17 2022' -A x64 -T ClangCL
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $CMake --build $BuildDirectory --config Release --parallel 8
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$cmakeCommand = (Get-Command $CMake).Source
$ctestCommand = Join-Path (Split-Path $cmakeCommand) 'ctest.exe'
& $ctestCommand --test-dir $BuildDirectory -C Release --output-on-failure
exit $LASTEXITCODE
