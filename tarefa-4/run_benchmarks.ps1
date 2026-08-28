param(
    [int64]$MemoryElements = 33554432,
    [int]$MemoryRepetitions = 4,
    [int64]$CpuElements = 1000000,
    [int]$CpuIterations = 200
)

$ErrorActionPreference = 'Stop'
$taskDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path

Push-Location $taskDirectory
try {
    gcc -O2 -Wall -Wextra -std=c99 -fopenmp memory_bound.c -o memory_bound.exe
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao compilar memory_bound.c' }

    gcc -O2 -Wall -Wextra -std=c99 -fopenmp cpu_bound.c -o cpu_bound.exe -lm
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao compilar cpu_bound.c' }

    $memoryOutput = & .\memory_bound.exe $MemoryElements $MemoryRepetitions
    $memoryExitCode = $LASTEXITCODE
    $memoryOutput | Write-Output
    $memoryOutput | Set-Content -Encoding utf8 resultados_memory.txt
    if ($memoryExitCode -ne 0) { throw 'Falha no benchmark memory-bound' }

    $cpuOutput = & .\cpu_bound.exe $CpuElements $CpuIterations
    $cpuExitCode = $LASTEXITCODE
    $cpuOutput | Write-Output
    $cpuOutput | Set-Content -Encoding utf8 resultados_cpu.txt
    if ($cpuExitCode -ne 0) { throw 'Falha no benchmark compute-bound' }

}
finally {
    Pop-Location
}
