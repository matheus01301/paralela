param(
    [long]$Amostras = 10000000,
    [int]$Threads = 0,
    [int]$Execucoes = 5,
    [long]$Semente = 2026
)

$ErrorActionPreference = 'Stop'
$taskDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path

Push-Location $taskDirectory
try {
    gcc -O2 -Wall -Wextra -std=c99 -fopenmp pi_monte_carlo.c -o pi_monte_carlo.exe
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao compilar pi_monte_carlo.c' }

    if ($Threads -le 0) {
        $Threads = [Environment]::ProcessorCount
    }

    $linhas = @(
        'Tarefa 6 - estimativa estocastica de pi'
        "Amostras: $Amostras | Threads: $Threads | Execucoes: $Execucoes | Semente: $Semente"
        ''
    )

    for ($i = 1; $i -le $Execucoes; $i++) {
        $linhas += "--- Execucao $i ---"
        $saida = & .\pi_monte_carlo.exe $Amostras $Threads $Semente
        if ($LASTEXITCODE -ne 0) { throw "Falha na execucao $i" }
        $linhas += $saida
        $linhas += ''
    }

    $linhas | Write-Output
    $linhas | Set-Content -Encoding utf8 resultados.txt
}
finally {
    Pop-Location
}
