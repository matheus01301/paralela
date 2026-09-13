param(
    [long]$Pontos = 20000000,
    [int]$Threads = 8,
    [int]$Execucoes = 5,
    [uint32]$Semente = 2026
)

$ErrorActionPreference = 'Stop'
$taskDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path

Push-Location $taskDirectory
try {
    gcc -O2 -Wall -Wextra -std=c11 -fopenmp pi_sincronizacao.c -o pi_sincronizacao.exe
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao compilar pi_sincronizacao.c' }

    $linhas = @(
        'Tarefa 10 - critical, atomic, contadores privados e reduction'
        "Pontos: $Pontos | Threads: $Threads | Execucoes: $Execucoes"
        ''
    )

    for ($execucao = 1; $execucao -le $Execucoes; $execucao++) {
        $linhas += "--- Execucao $execucao ---"
        $sementeAtual = $Semente + $execucao - 1
        $saida = & .\pi_sincronizacao.exe $Pontos $Threads $sementeAtual
        if ($LASTEXITCODE -ne 0) { throw "Falha na execucao $execucao" }
        $linhas += $saida
        if ($execucao -lt $Execucoes) { $linhas += '' }
    }

    $linhas | Write-Output
    $linhas | Set-Content -Encoding utf8 resultados.txt
}
finally {
    Pop-Location
}
