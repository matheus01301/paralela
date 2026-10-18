# Produto matriz-vetor com MPI

## O que o programa faz

**Conta:** `y[i] = soma(A[i,j] × x[j])`, para `j` de `0` até `N-1`. Cada linha
produz um elemento de `y` e pode ser calculada separadamente. A matriz fica em
um vetor de memória, com as colunas de cada linha em sequência.

**Divisão do trabalho:** o processo 0 cria `A` e `x`. Com `P` processos, cada um
recebe `ceil(M/P)` linhas. Se sobram posições, `calloc` deixa essas linhas em
zero; o processo 0 usa apenas os primeiros `M` resultados. Isso permite usar
`MPI_Scatter` e `MPI_Gather`, que enviam a mesma quantidade a cada processo.

| chamada | papel no programa |
|---|---|
| `MPI_Comm_rank` / `MPI_Comm_size` | Identificam o processo e o total de processos. |
| `MPI_Scatter` | Reparte as linhas de `A` entre os processos. |
| `MPI_Bcast` | Entrega `x` inteiro a todos: toda linha precisa do vetor completo. |
| Laços `i` e `j` | `i` percorre as linhas locais; `j` faz a soma dos produtos. |
| `MPI_Gather` | Reúne as partes de `y` em ordem no processo 0. |

**Teste de correção:** com `M=5`, `N=3`, a primeira linha é `1,2,3`, então
`y[0]=6`; a última é `5,6,7`, então `y[4]=18`. Os dois valores saíram iguais
com 1, 2 e 8 processos. O caso de 8 processos também testa linhas extras.

## Tempos medidos

**Ambiente:** NPAD, nó `r2n09`, partição `amd-512`, um nó exclusivo;
GCC 8.5.0, Open MPI 4.1.6, compilação com `-O2 -Wall -Wextra -std=c11`.
Para cada tamanho e número de processos, foram feitas cinco execuções;
a tabela mostra a **mediana**, em segundos. `MPI_Barrier` alinha o início;
`MPI_Wtime` mede de antes do `Scatter` até depois do `Gather` no processo 0.
A criação de `A` e `x` fica fora desse intervalo.

| Matriz | 1 processo | 2 processos | 4 processos | 8 processos |
|---:|---:|---:|---:|---:|
| 2048 × 2048 | 0,008543 | 0,014674 | 0,021900 | 0,023500 |
| 4096 × 4096 | 0,033235 | 0,040298 | 0,040465 | 0,043019 |
| 8192 × 8192 | 0,131454 | **0,120667** | 0,132492 | 0,126684 |

**Leitura:** em 2048², 8 processos levam cerca de 2,75 vezes o tempo de 1.
Em 8192², 2 processos dão ganho pequeno: `T(1)/T(2) ≈ 1,09`.
Os tempos de 4 e 8 processos nessa matriz são próximos do tempo com 1.
Mais processos reduzem as linhas calculadas por cada um, mas distribuir `A`
e reunir `y` também custa tempo. Essa é uma explicação provável para o ganho
limitado; o programa não mediu cada fase separadamente.

**Para defender:** `rank` é o número de cada processo; as três chamadas de
comunicação são *coletivas*, portanto todos os processos as executam na mesma
ordem. *Speedup* compara `T(1)/T(P)` para o mesmo tamanho. Aqui, `T(1)` já inclui
as chamadas MPI com um processo, não é uma versão sequencial separada.

<div class="quebra"></div>

## Código usado

{{CODIGO:matriz_vetor_mpi.c}}
