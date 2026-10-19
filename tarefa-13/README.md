# Tarefa 13 — afinidade de threads e escalabilidade

O código de Navier-Stokes da Tarefa 11 (`schedule(static)`, inicialização em
paralelo) rodado no mesmo nó da Tarefa 12 (`r2n19`), mudando só a afinidade das
threads.

| eixo | valores testados |
|---|---|
| `OMP_PROC_BIND` (com `OMP_PLACES=cores`) | `false`, `true`, `close`, `spread`, `master` |
| sistema operacional | `taskset -c 0-15`, `numactl --interleave=all` |

Cada thread informa a CPU em que está (`sched_getcpu`), e o programa mede o tempo
com 1 a 128 threads.

## Executar

```bash
sbatch job_npad.sh          # no NPAD, no r2n19; saida em resultados.txt

# direto, uma configuracao
gcc -O2 -Wall -Wextra -std=c11 -fopenmp afinidade.c -o afinidade -lm
OMP_PROC_BIND=spread OMP_PLACES=cores ./afinidade spread 128
```

## Arquivos

| Arquivo | Conteúdo |
|---|---|
| `afinidade.c` | simulação, medição e mapa thread → CPU |
| `job_npad.sh` | script Slurm com as sete configurações |
| `resultados.txt` | saída usada no relatório |
| `relatorio/graficos.py` | gera `tempo.svg` |
| `relatorio/relatorio.pdf` | relatório com gráfico e código realçado |
| `guia_apresentacao.md` | perguntas para a defesa oral |
