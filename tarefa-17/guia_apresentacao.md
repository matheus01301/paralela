# Apoio para a defesa — Tarefa 17

## Ordem para explicar

1. **Conta:** `y[i]` é o produto escalar da linha `i` de `A` por `x`.
2. **Divisão:** cada processo recebe `ceil(N/P)` colunas e o mesmo número de elementos de `x`. Colunas extras são zeros.
3. **Tipo derivado:** `MPI_Type_vector(M, colunas_locais, colunas_distribuidas, MPI_DOUBLE, ...)` escolhe um bloco em cada linha. `MPI_Type_create_resized` fixa o avanço entre blocos em `colunas_locais × sizeof(double)` bytes.
4. **Cálculo:** cada processo faz uma soma parcial para **todas** as `M` linhas. `MPI_Reduce` com `MPI_SUM` junta as partes.
5. **Tempo:** `MPI_Wtime` mede distribuição, cálculo e redução. Inicialização da matriz fica fora.
6. **Comparação:** a versão por linhas envia trechos contíguos de `A` e reúne partes de `y`; a por colunas precisa descrever blocos separados na origem e reduzir vetores inteiros.

**Medianas no NPAD:** em 8192² e 4 processos, linhas = **0,131396 s**;
colunas = **0,286481 s** (2,18 vezes). Em 2048² e 8 processos, os tempos
ficaram praticamente iguais (0,023278 s e 0,023201 s). São observações
deste experimento, não uma regra para toda máquina ou tamanho.

## Exemplo para desenhar

Com `M=2`, `N=5` e `P=2`, cada processo recebe 3 colunas. A matriz é guardada
com 6 posições por linha: a última é zero. O processo 0 recebe as colunas
`0,1,2`; o 1 recebe `3,4,5`. `MPI_Type_vector` salta 6 posições para chegar à
próxima linha. `MPI_Type_create_resized` faz o `Scatter` avançar 3 posições
para começar o bloco do processo 1.

## Nomes do código

| nome | papel |
|---|---|
| `mpi.h` | Declara tipos e funções MPI. |
| `stdio.h`, `stdlib.h` | Impressão, `atoi`, `malloc`, `calloc` e `free`. |
| `argc`, `argv`; `m`, `n` | Argumentos do programa; linhas e colunas. |
| `processo`, `processos` | `rank` atual e total de processos. |
| `colunas_locais`, `colunas_distribuidas` | Largura de cada bloco e largura com zeros extras. |
| `a`, `x`, `y` | Dados completos no processo 0. |
| `a_local`, `x_local`, `y_local` | Dados recebidos e contribuição parcial do processo. |
| `tipo_colunas`, `tipo_redimensionado` | Descrição das posições na origem e seu avanço no `Scatter`. |
| `i`, `j`, `soma` | Linha, coluna local e soma parcial. |
| `inicio`, `tempo` | Marcas de tempo de parede em segundos. |

`MPI_Init` inicia o MPI; `MPI_Finalize` encerra. `MPI_Type_commit` prepara o
tipo para comunicação e `MPI_Type_free` o libera. `MPI_Abort` encerra todos se
faltar memória. `MPI_COMM_WORLD` inclui os processos da execução;
`MPI_DOUBLE` informa o tipo básico; `MPI_SUM` informa como juntar as parcelas.

## Perguntas prováveis

- **Por que `Type_vector`?** As colunas de um bloco aparecem em cada linha, com um salto de uma linha inteira até o próximo bloco.
- **Por que `create_resized`?** O `Scatter` deve passar para o próximo grupo de colunas da *primeira* linha, e não depois da última linha do grupo anterior.
- **O bloco recebido também fica espaçado?** Não. `a_local` recebe os valores compactados; o laço local percorre cada linha em sequência.
- **Por que `Reduce` em vez de `Gather`?** Cada processo calcula apenas parte de cada `y[i]`; essas partes precisam ser somadas.
- **O que é localidade espacial aqui?** Ler posições próximas aproveita a mesma linha de cache. A origem é menos simples de percorrer por colunas; o bloco recebido é compacto.
- **Onde há localidade temporal?** O mesmo `x_local` é reutilizado para cada uma das `M` linhas.
- **Por que 1 processo também usa o tipo derivado?** Mantém o mesmo algoritmo e dá uma base `T(1)` para calcular speedup.
