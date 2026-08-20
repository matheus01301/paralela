# Tarefa 2 — Multiplicação de matriz por vetor

A tarefa compara duas versões de `y = A × x` em C. A primeira percorre a matriz
por linhas e a segunda por colunas. A quantidade de operações é a mesma; o que
muda é a ordem de acesso à memória.

## Compilar e executar

```bash
gcc -O2 -Wall -Wextra -std=c99 mxv.c -o mxv.exe -lm
./mxv.exe
```

O programa utiliza `clock_gettime(CLOCK_MONOTONIC)` para medir o tempo de parede e
testa matrizes quadradas de `N = 64` até `N = 4096`.

## Arquivos

| arquivo | conteúdo |
|---|---|
| `mxv.c` | código C da tarefa |
| `relatorio/relatorio.pdf` | relatório para entrega |
| `relatorio/relatorio.md` | texto usado para gerar o PDF |
| `guia_apresentacao.md` | resumo e perguntas para a explicação presencial |

No computador utilizado, os tempos passam a divergir de forma clara em `N = 512`.
O acesso por linhas aproveita melhor a memória cache porque percorre posições
consecutivas; o acesso por colunas realiza saltos maiores pela memória.
