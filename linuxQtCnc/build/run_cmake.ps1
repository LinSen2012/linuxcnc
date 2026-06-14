$ErrorActionPreference = 'Continue'
$cmakePath = 'C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$srcDir = 'd:\cnclx\linuxcnc\linuxQtCnc'
Set-Location $srcDir
if (-not (Test-Path build)) { New-Item -ItemType Directory build | Out-Null }
Set-Location build
Write-Host "Running cmake..."
& $cmakePath -G "Visual Studio 15 2017 Win64" $srcDir 2>&1 | Out-File -FilePath cmake_output.txt -Encoding utf8
Write-Host "CMake exit: $LASTEXITCODE"
Get-Content cmake_output.txt -Tail 80
