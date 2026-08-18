# Programação Paralela — tarefas

Exercícios da disciplina, implementados em **C** e, onde faz sentido comparar as duas
plataformas, também em **TypeScript** sobre Node.

## Estrutura

| pasta | tarefa |
|---|---|
| [`tarefa-1/`](tarefa-1/) | Aproximação de π por séries: acurácia × iterações, tempo de execução e o limite de precisão do `double` |
| [`tarefa-2/`](tarefa-2/) | Multiplicação matriz-vetor: acesso por linhas × por colunas, e o efeito do padrão de acesso à memória |

Cada tarefa tem seu próprio `README.md` com os resultados medidos. A tarefa 1 tem
ainda o relatório em PDF, com o código-fonte realçado, em
[`tarefa-1/relatorio/`](tarefa-1/relatorio/).

## Protocolo de medição

Fixo para todos os programas, para que os números sejam comparáveis entre si:

- **Tempo de parede** para calcular *speedup* — `QueryPerformanceCounter` no Windows,
  `clock_gettime(CLOCK_MONOTONIC)` no POSIX, `performance.now()` no Node. Nunca
  `Date.now()`, que tem resolução de ~1 ms e pode andar para trás com ajuste de NTP.
  O tempo de CPU aparece em coluna separada.
- **Mediana de várias execuções**, nunca média.
- **Aquecimento de JIT obrigatório** nas versões TypeScript.
- **`-O2` sempre declarado.** Com `-O0` os números não são comparáveis.

## Ambiente das medições

| | |
|---|---|
| CPU | Intel i7-13650HX — 14 núcleos (6 P-cores + 8 E-cores), 20 threads |
| SO | Windows 11 |
| Compilador C | gcc 16.1.0 (MinGW-w64 UCRT), OpenMP 5.2 |
| Runtime JS | Node v22.19.0 |

## Como compilar

```bash
# Tarefa 1 — série serial, versão paralela, escalonamento e overhead
cd tarefa-1
gcc -O2 -Wall -Wextra -std=c99 pi_serie.c -o pi_serie.exe -lm
gcc -O2 -Wall -Wextra -fopenmp   pi_omp.c   -o pi_omp.exe   -lm
gcc -O2 -Wall -Wextra -fopenmp   sched_test.c -o sched_test.exe -lm
node --experimental-strip-types pi_serie.ts 9

# Tarefa 2 — multiplicação matriz-vetor
cd ../tarefa-2
gcc -O2 -Wall -Wextra -std=c99 mxv.c -o mxv.exe -lm
```

Todos os fontes em C compilam sem emitir nenhum aviso com `-Wall -Wextra`.

## Relatório em PDF

O relatório da tarefa 1 é gerado a partir do Markdown, com realce de sintaxe do
código, sem depender de pandoc nem de LaTeX:

```bash
cd tarefa-1/relatorio
python build_pdf.py       # markdown-it-py + Pygments + Chrome headless
```
