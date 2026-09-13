param(
    [long]$Insercoes = 200000,
    [int]$Threads = 8,
    [int]$Listas = 8,
    [int]$Execucoes = 5,
    [uint32]$Semente = 2026
)

$ErrorActionPreference = 'Stop'
$taskDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path

Push-Location $taskDirectory
try {
    gcc -O2 -Wall -Wextra -std=c11 -fopenmp listas_insercoes.c -o listas_insercoes.exe
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao compilar listas_insercoes.c' }

    $linhas = @(
        'Tarefa 9 - insercoes concorrentes em listas encadeadas'
        "N: $Insercoes | Threads: $Threads | Listas: $Listas | Execucoes: $Execucoes"
        ''
    )

    for ($execucao = 1; $execucao -le $Execucoes; $execucao++) {
        $linhas += "--- Execucao $execucao ---"
        $sementeAtual = $Semente + $execucao - 1
        $saida = & .\listas_insercoes.exe $Insercoes $Threads $Listas $sementeAtual
        if ($LASTEXITCODE -ne 0) { throw "Falha na execucao $execucao" }
        $linhas += $saida
        $linhas += ''
    }

    $linhas | Write-Output
    $linhas | Set-Content -Encoding utf8 resultados.txt
}
finally {
    Pop-Location
}
