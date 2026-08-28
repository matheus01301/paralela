# Programação Paralela — tarefas

Exercícios da disciplina, implementados em **C** e, onde faz sentido comparar as duas
plataformas, também em **TypeScript** sobre Node.

## Estrutura

| pasta | tarefa |
|---|---|
| [`tarefa-1/`](tarefa-1/) | Aproximação de π por séries: acurácia × iterações, tempo de execução e o limite de precisão do `double` |
| [`tarefa-2/`](tarefa-2/) | Multiplicação matriz-vetor: acesso por linhas × por colunas, e o efeito do padrão de acesso à memória |
| [`tarefa-3/`](tarefa-3/) | Paralelismo ao nível de instrução: dependências, múltiplos acumuladores e otimizações do compilador |
| [`tarefa-4/`](tarefa-4/) | OpenMP e Roofline Model: programas limitados pela memória e pela CPU |

Cada tarefa tem seu próprio `README.md`, um relatório em PDF com o código-fonte
realçado e um `guia_apresentacao.md` com perguntas para a explicação presencial.

## Protocolo de medição

Fixo para todos os programas, para que os números sejam comparáveis entre si:

- **Tempo de parede** para calcular *speedup* — `QueryPerformanceCounter` no Windows,
  `clock_gettime(CLOCK_MONOTONIC)` no POSIX, `performance.now()` no Node. Nunca
  `Date.now()`, que tem resolução de ~1 ms e pode andar para trás com ajuste de NTP.
  O tempo de CPU aparece em coluna separada.
- **Mediana de várias execuções**, nunca média.
- **Aquecimento de JIT obrigatório** nas versões TypeScript.
- **Nível de otimização sempre declarado.** Nas tarefas 1, 2 e 4 foi usado `-O2`;
  a tarefa 3 compara explicitamente `-O0`, `-O2` e `-O3`.

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

# Tarefa 3 — ILP e níveis de otimização
cd ../tarefa-3
./executar_testes.ps1

# Tarefa 4 — benchmarks memory-bound e compute-bound com OpenMP
cd ../tarefa-4
gcc -O2 -Wall -Wextra -std=c99 -fopenmp memory_bound.c -o memory_bound.exe
gcc -O2 -Wall -Wextra -std=c99 -fopenmp cpu_bound.c -o cpu_bound.exe -lm
```

Todos os fontes em C compilam sem emitir nenhum aviso com `-Wall -Wextra`.

## Relatórios em PDF

Os relatórios são gerados a partir do Markdown, com realce de sintaxe do código,
sem depender de pandoc nem de LaTeX:

```bash
cd tarefa-1/relatorio
python build_pdf.py       # markdown-it-py + Pygments + Chrome headless

cd ../../tarefa-2/relatorio
python build_pdf.py

cd ../../tarefa-3/relatorio
python build_pdf.py

cd ../../tarefa-4/relatorio
python build_pdf.py
```
