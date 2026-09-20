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
| [`tarefa-5/`](tarefa-5/) | Contagem de primos com OpenMP: correção e distribuição de carga |
| [`tarefa-6/`](tarefa-6/) | Estimativa estocástica de π: condição de corrida, `critical` e escopo de dados no OpenMP |
| [`tarefa-7/`](tarefa-7/) | Lista encadeada com tarefas OpenMP: `task`, `single`, captura de dados e sincronização |
| [`tarefa-8/`](tarefa-8/) | Monte Carlo com `rand`/`rand_r`: coerência de cache e falso compartilhamento |
| [`tarefa-9/`](tarefa-9/) | Inserções concorrentes em listas encadeadas: `critical` nomeado e locks explícitos |
| [`tarefa-10/`](tarefa-10/) | Monte Carlo com `critical`, `atomic`, privatização e `reduction` |
| [`tarefa-11/`](tarefa-11/) | Difusão viscosa (Navier-Stokes sem pressão) por diferenças finitas: impacto de `schedule` e `collapse` |
| [`tarefa-12/`](tarefa-12/) | Escalabilidade forte e fraca do Navier-Stokes: gargalos e cinco versões otimizadas |
| [`tarefa-13/`](tarefa-13/) | Afinidade de threads: `OMP_PROC_BIND`, `OMP_PLACES`, `numactl` e `GOMP_CPU_AFFINITY` |

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

Tarefas 1 a 10, máquina local:

| | |
|---|---|
| CPU | Intel i7-13650HX — 14 núcleos (6 P-cores + 8 E-cores), 20 threads |
| SO | Windows 11 |
| Compilador C | gcc 16.1.0 (MinGW-w64 UCRT), OpenMP 5.2 |
| Runtime JS | Node v22.19.0 |

A partir da Tarefa 11, **NPAD/UFRN**:

| | |
|---|---|
| Máquina | partição `amd-512`, nó `r2n19` (tarefas 12 e 13), alocado com `--exclusive` |
| CPU | 2 sockets × 64 núcleos, 2 threads por núcleo (128 físicos, 256 lógicos) |
| Memória | 512 GB |
| SO | Linux 4.18 (RHEL 8) |
| Compilador C | gcc 8.5.0 |
| CPU (detalhe) | 2 × AMD EPYC 7713, 8 domínios NUMA, 32 MB de L3 por CCX de 8 núcleos |
| Escalonador | Slurm (`sbatch job_npad.sh`) |
| Threads | `OMP_PLACES=cores`, `OMP_PROC_BIND=close` |

Acesso: `ssh npad` (atalho em `~/.ssh/config` para
`mrmarinho@sc2.npad.ufrn.br` na porta 4422).

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

# Tarefa 5 — contagem sequencial e paralela de números primos
cd ../tarefa-5
gcc -O2 -Wall -Wextra -std=c99 -fopenmp primos.c -o primos.exe

# Tarefa 6 — estimativa estocástica de π e cláusulas OpenMP
cd ../tarefa-6
gcc -O2 -Wall -Wextra -std=c99 -fopenmp pi_monte_carlo.c -o pi_monte_carlo.exe

# Tarefa 7 — lista encadeada e tarefas OpenMP
cd ../tarefa-7
gcc -O2 -Wall -Wextra -std=c99 -fopenmp lista_tarefas.c -o lista_tarefas.exe

# Tarefa 8 — rand, rand_r e falso compartilhamento (Linux/WSL)
cd ../tarefa-8
gcc -O2 -Wall -Wextra -std=c11 -fopenmp pi_rand_openmp.c -o pi_rand_openmp

# Tarefa 9 — listas encadeadas com critical nomeado e locks explícitos
cd ../tarefa-9
gcc -O2 -Wall -Wextra -std=c11 -fopenmp listas_insercoes.c -o listas_insercoes.exe

# Tarefa 10 — comparação de mecanismos de sincronização
cd ../tarefa-10
gcc -O2 -Wall -Wextra -std=c11 -fopenmp pi_sincronizacao.c -o pi_sincronizacao.exe

# Tarefa 11 — difusão viscosa, schedule e collapse (NPAD)
cd ../tarefa-11
gcc -O2 -Wall -Wextra -std=c11 -fopenmp difusao_viscosa.c -o difusao_viscosa -lm
sbatch job_npad.sh        # no NPAD

# Tarefa 12 — escalabilidade forte e fraca (NPAD)
cd ../tarefa-12
gcc -O2 -Wall -Wextra -std=c11 -fopenmp escalabilidade.c -o escalabilidade -lm
sbatch job_npad.sh        # no NPAD

# Tarefa 13 — afinidade de threads (NPAD, mesmo nó da 12)
cd ../tarefa-13
gcc -O2 -Wall -Wextra -std=c11 -fopenmp afinidade.c -o afinidade -lm
sbatch job_npad.sh        # no NPAD
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

cd ../../tarefa-5/relatorio
python build_pdf.py

cd ../../tarefa-6/relatorio
python build_pdf.py

cd ../../tarefa-7/relatorio
python build_pdf.py

cd ../../tarefa-8/relatorio
python build_pdf.py

cd ../../tarefa-9/relatorio
python build_pdf.py

cd ../../tarefa-10/relatorio
python build_pdf.py

cd ../../tarefa-11/relatorio
python build_pdf.py

cd ../../tarefa-12/relatorio
python graficos.py        # gera os SVG a partir de resultados.txt
python build_pdf.py

cd ../../tarefa-13/relatorio
python graficos.py
python build_pdf.py
```
