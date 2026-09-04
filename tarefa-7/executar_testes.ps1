param(
    [int]$Threads = 4,
    [int]$Execucoes = 3
)

$ErrorActionPreference = 'Stop'
$taskDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path

Push-Location $taskDirectory
try {
    gcc -O2 -Wall -Wextra -std=c99 -fopenmp lista_tarefas.c -o lista_tarefas.exe
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao compilar lista_tarefas.c' }

    $linhas = @(
        'Tarefa 7 - tarefas OpenMP em uma lista encadeada'
        "Threads: $Threads | Execucoes: $Execucoes"
        ''
    )

    for ($i = 1; $i -le $Execucoes; $i++) {
        $linhas += "--- Execucao $i ---"
        $saida = & .\lista_tarefas.exe $Threads
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
