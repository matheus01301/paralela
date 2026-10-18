# Produto matriz-vetor distribuído por colunas

## Conta e divisão

**Conta:** `y[i] = soma(A[i,j] × x[j])`. Nesta versão, cada processo recebe um
grupo de colunas de `A` e a parte correspondente de `x`. Ele calcula uma
**contribuição parcial para cada uma das `M` linhas**. `MPI_Reduce` com `MPI_SUM`
soma essas contribuições e entrega `y` completo ao processo 0.

**Colunas extras:** `colunas_locais = ceil(N/P)`. O processo 0 guarda a matriz
com `P × colunas_locais` posições por linha; as que passam de `N` valem zero.
O mesmo vale para `x`. Assim, cada processo recebe um bloco igual, mesmo quando
`N` não é múltiplo de `P`.

| chamada | palavra-chave para explicar |
|---|---|
| `MPI_Type_vector(M, colunas_locais, colunas_distribuidas, ...)` | Seleciona um grupo de colunas em cada uma das `M` linhas. O salto entre linhas é `colunas_distribuidas`. |
| `MPI_Type_create_resized(..., colunas_locais × sizeof(double), ...)` | Faz o próximo bloco do `Scatter` começar nas colunas seguintes da primeira linha. Não muda os valores selecionados. |
| 1º `MPI_Scatter` | Usa o tipo derivado na origem; `a_local` recebe `M × colunas_locais` valores **compactados**. |
| 2º `MPI_Scatter` | Envia o trecho de `x` que corresponde às colunas recebidas. |
| `MPI_Reduce` / `MPI_SUM` | Soma, posição a posição, os vetores `y_local` de tamanho `M`. |

**Correção:** para `M=5`, `N=3`, saíram `y[0]=6` e `y[4]=18` com 1, 2 e 8
processos. O teste retangular `M=3`, `N=5` passou com 2 e 8 processos.

## Linhas × colunas: tempo e memória

**Medição:** mesmo job, nó exclusivo `r2n09` do NPAD, GCC 8.5.0,
Open MPI 4.1.6 e `-O2`. Foram cinco rodadas alternadas por configuração;
a tabela traz a **mediana em segundos**. `MPI_Wtime` mede do `Scatter`
até o `Gather` ou `Reduce`. A criação dos dados fica fora do cronômetro.

| Matriz | Processos | Linhas (T16) | Colunas (T17) | Colunas / linhas |
|---:|---:|---:|---:|---:|
| 2048² | 1 | 0,008515 | 0,009507 | 1,12× |
| 2048² | 2 | 0,014877 | 0,019313 | 1,30× |
| 2048² | 4 | 0,021083 | 0,027979 | 1,33× |
| 2048² | 8 | 0,023278 | 0,023201 | 1,00× |
| 4096² | 1 | 0,033708 | 0,035740 | 1,06× |
| 4096² | 2 | 0,040409 | 0,051417 | 1,27× |
| 4096² | 4 | 0,040321 | 0,081891 | 2,03× |
| 4096² | 8 | 0,050097 | 0,057542 | 1,15× |
| 8192² | 1 | 0,131222 | 0,139849 | 1,07× |
| 8192² | 2 | 0,127577 | 0,179324 | 1,41× |
| 8192² | 4 | 0,131396 | 0,286481 | 2,18× |
| 8192² | 8 | 0,123437 | 0,228917 | 1,85× |

**Leitura:** colunas foi mais lenta na maioria dos casos. Em 2048²/8,
os tempos empataram; em 8192²/4, colunas levou **2,18 vezes** o tempo.
A diferença não cresce sempre com `P`.

**Memória e comunicação:** em `A`, linhas são contíguas; colunas exigem
saltos entre linhas na origem. O `Scatter` compacta os blocos recebidos:
**o cálculo local continua contíguo** e reutiliza `x_local` a cada linha.
A T17 ainda reduz `M` valores por processo; a T16 reúne só sua parte de `y`.
Esses custos ajudam a explicar a diferença, mas não foram cronometrados
separadamente. Em 8192²/4, colunas variou de 0,267 a 0,350 s: pequenas
diferenças de tempo não sustentam uma conclusão.

<div class="quebra"></div>

## Código usado

{{CODIGO:matriz_vetor_colunas_mpi.c}}
