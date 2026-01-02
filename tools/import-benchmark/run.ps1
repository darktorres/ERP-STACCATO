$env:PATH = "C:\Qt\5.15.2\msvc2019_64\bin;" + $env:PATH
$exe = "$PSScriptRoot\release\import-benchmark.exe"
Write-Host "Running: $exe $args"
& $exe @args 2>&1 | Tee-Object -FilePath "$PSScriptRoot\output.log"
Write-Host "Exit code: $LASTEXITCODE"
