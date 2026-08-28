param(
    [int]$Limite = 10000000,
    [int]$Threads = 0,
    [int]$Execucoes = 5
)

$ErrorActionPreference = 'Stop'
$taskDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path

Push-Location $taskDirectory
try {
    gcc -O2 -Wall -Wextra -std=c99 -fopenmp primos.c -o primos.exe
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao compilar primos.c' }

    if ($Threads -le 0) {
        $Threads = [Environment]::ProcessorCount
    }

    $linhas = @(
        "Tarefa 5 - contagem de primos"
        "Limite: $Limite | Threads: $Threads | Execucoes: $Execucoes"
        ""
    )

    for ($i = 1; $i -le $Execucoes; $i++) {
        $linhas += "--- Execucao $i ---"
        $saida = & .\primos.exe $Limite $Threads
        if ($LASTEXITCODE -ne 0) { throw "Falha na execucao $i" }
        $linhas += $saida
        $linhas += ""
    }

    $linhas | Write-Output
    $linhas | Set-Content -Encoding utf8 resultados.txt
}
finally {
    Pop-Location
}
