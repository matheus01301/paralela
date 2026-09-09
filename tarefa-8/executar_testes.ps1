param(
    [long]$Pontos = 10000000,
    [int]$Threads = 0,
    [int]$Execucoes = 5,
    [long]$Semente = 2026
)

$ErrorActionPreference = 'Stop'
$taskDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$linuxDirectory = (wsl.exe wslpath -a ($taskDirectory -replace '\\', '/')).Trim()

if ($Threads -le 0) {
    $Threads = [int](wsl.exe nproc).Trim()
}

$comandoCompilar = "cd '$linuxDirectory' && gcc -O2 -Wall -Wextra -std=c11 -fopenmp pi_rand_openmp.c -o pi_rand_openmp"
wsl.exe sh -lc $comandoCompilar
if ($LASTEXITCODE -ne 0) { throw 'Falha ao compilar pi_rand_openmp.c no WSL' }

$linhas = @(
    'Tarefa 8 - rand, rand_r e falso compartilhamento'
    "Pontos: $Pontos | Threads: $Threads | Execucoes: $Execucoes | Semente: $Semente"
    'Ambiente: Ubuntu 24.04 no WSL 2, glibc'
    ''
)

for ($i = 1; $i -le $Execucoes; $i++) {
    $linhas += "--- Execucao $i ---"
    $comandoExecutar = "cd '$linuxDirectory' && ./pi_rand_openmp $Pontos $Threads $Semente"
    $saida = wsl.exe sh -lc $comandoExecutar
    if ($LASTEXITCODE -ne 0) { throw "Falha na execucao $i" }
    $linhas += $saida
    if ($i -lt $Execucoes) {
        $linhas += ''
    }
}

$linhas += 'Fim dos resultados.'

$linhas | Write-Output
$linhas | Set-Content -Encoding utf8 (Join-Path $taskDirectory 'resultados.txt')
