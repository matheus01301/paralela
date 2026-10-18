# Apoio para a defesa — Tarefa 16

## Sequência para explicar

1. **Conta:** cada elemento `y[i]` é a soma dos produtos da linha `i` de `A` pelos elementos de `x`.
2. **Divisão:** as linhas são independentes. Cada processo recebe `ceil(M/P)` linhas consecutivas; linhas extras valem zero.
3. **Dados:** `Scatter` reparte `A`; `Bcast` copia `x` inteiro; `Gather` junta `y` na ordem dos processos.
4. **Tempo:** `MPI_Barrier` alinha o começo; `MPI_Wtime` mede comunicação e cálculo até o fim do `Gather` no processo 0. A inicialização fica fora.
5. **Resultado:** comparar as medianas de cinco rodadas por tamanho e número de processos. Crescer `P` também aumenta o custo de distribuir `A`.

No NPAD: 2048² passou de **0,008543 s** (1 processo) para **0,023500 s**
(8 processos). Em 8192², 2 processos tiveram **0,120667 s** contra
**0,131454 s** com 1: ganho de apenas **1,09×**. Não dizer que mais processos
sempre melhoram o tempo.

## Nomes do código

| nome | papel |
|---|---|
| `mpi.h` | Declara as funções e os tipos do MPI. |
| `stdio.h` / `stdlib.h` | Impressão; conversão dos argumentos, memória e saída. |
| `argc`, `argv` | Quantidade de argumentos e textos recebidos na linha de comando. `atoi` converte `M` e `N`. |
| `processo`, `processos` | Identificação de quem executa e quantidade total (`MPI_Comm_rank` e `MPI_Comm_size`). |
| `m`, `n` | Linhas e colunas da matriz; `x` tem `n` posições e `y` tem `m` válidas. |
| `linhas_locais`, `linhas_distribuidas` | Tamanho fixo do bloco de cada processo e total após completar as linhas. |
| `a`, `x`, `y` | Matriz e vetor de entrada; vetor de saída, reunido no processo 0. |
| `a_local`, `y_local` | Bloco de linhas recebido e resultados calculados pelo processo atual. |
| `i`, `j`, `soma` | Linha, coluna e acumulador do produto escalar. |
| `inicio`, `tempo` | Instante inicial e duração em segundos, obtidos com `MPI_Wtime`. |

`MPI_Init` inicia o ambiente MPI; `MPI_Finalize` encerra. `MPI_Abort` encerra
todos se faltar memória. `malloc` reserva memória, `calloc` reserva e zera as
linhas extras, `free` libera. `printf` mostra o resultado; `fprintf` mostra
erro. `MPI_DOUBLE` informa o tipo enviado; `MPI_COMM_WORLD` é o grupo de todos
os processos desta execução.

## Perguntas prováveis

- **Por que distribuir por linhas?** Cada linha gera exatamente um elemento de `y`; não precisa somar resultados entre processos.
- **Por que o vetor vai inteiro?** Todas as linhas precisam de todos os `N` elementos de `x`.
- **Por que completar com zeros?** `MPI_Scatter` e `MPI_Gather` usam a mesma quantidade por processo. Assim aceitamos qualquer `M` sem mudar a chamada.
- **O que é `rank`?** Número do processo dentro de `MPI_COMM_WORLD`; o rank 0 organiza entrada e saída.
- **O que é comunicação coletiva?** Chamada feita por todos os processos do comunicador, como `Scatter`, `Bcast` e `Gather`.
- **O que é speedup?** `T(1)/T(P)` para o mesmo tamanho de matriz. Acima de 1 significa execução mais rápida que com um processo.
- **Por que mais processos podem piorar?** Distribuir a matriz custa tempo e todos disputam a memória do mesmo nó.
- **Qual a diferença entre `malloc` e `calloc` aqui?** `calloc` zera as linhas extras de `A`; `malloc` reserva os outros vetores, que serão preenchidos antes do uso.

Se alguma resposta não sair com palavras próprias, volte ao trecho do código correspondente e explique primeiro com um exemplo de `M=5`, `N=3` e `P=2`.
