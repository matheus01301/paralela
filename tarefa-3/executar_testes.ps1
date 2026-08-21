$ErrorActionPreference = "Stop"

$gccEncontrado = Get-Command gcc -ErrorAction SilentlyContinue
$gcc = if ($gccEncontrado) {
    $gccEncontrado.Source
} elseif (Test-Path "C:\gcc16\bin\gcc.exe") {
    "C:\gcc16\bin\gcc.exe"
} else {
    throw "GCC não encontrado. Adicione-o ao PATH antes de executar os testes."
}
$niveis = 0, 2, 3
$testes = "inicializacao", "dependente", "multipla2", "multipla4", "multipla8"
$medicoes = @()

foreach ($nivel in $niveis) {
    & $gcc "-O$nivel" -Wall -Wextra -std=c99 ilp.c -o "ilp_O$nivel.exe"
    if ($LASTEXITCODE -ne 0) {
        throw "Falha ao compilar com -O$nivel"
    }
}

foreach ($nivel in $niveis) {
    foreach ($teste in $testes) {
        $tempos = @()

        for ($amostra = 1; $amostra -le 3; $amostra++) {
            $saida = & ".\ilp_O$nivel.exe" $teste
            $linhaTempo = $saida | Select-String "Tempo por laco:"
            $tempo = [double]::Parse(
                ($linhaTempo -replace ".*Tempo por laco: ([0-9.]+) s.*", '$1'),
                [Globalization.CultureInfo]::InvariantCulture
            )
            $tempos += $tempo
        }

        $ordenados = $tempos | Sort-Object
        $medicoes += [pscustomobject]@{
            Otimizacao = "O$nivel"
            Teste = $teste
            MedianaSegundos = $ordenados[1]
        }
    }
}

$medicoes | Format-Table -AutoSize
