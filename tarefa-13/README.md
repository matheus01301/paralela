# Tarefa 13 — afinidade de threads e escalabilidade

Mesmo código da Tarefa 12 (versão `v4`), mesmo nó (`r2n19`), variando apenas a
política de afinidade das threads. Doze configurações, do OpenMP e do sistema
operacional.

| eixo | valores testados |
|---|---|
| `OMP_PROC_BIND` | `false`, `master`, `close`, `spread` |
| `OMP_PLACES` | `threads`, `cores`, `sockets`, `numa_domains` (não suportado no gcc 8.5) |
| sistema operacional | `GOMP_CPU_AFFINITY`, `numactl --interleave=all`, `numactl --cpunodebind` |

O programa não supõe onde as threads foram parar: cada uma chama `sched_getcpu()`
dentro da região paralela, e o resultado é cruzado com `/sys` para contar quantos
domínios de L3, nós NUMA e CPUs distintas foram de fato ocupados.

**Resultado:** `spread` é 2,4× mais rápido que `close` em 16 threads, e elimina o
platô de escalabilidade encontrado na Tarefa 12 — que portanto era consequência da
afinidade fixada, não do código. `master` degrada 16,6× em relação ao sequencial.

## Executar

```bash
sbatch job_npad.sh
cat slurm-<JobID>.out

# direto, uma configuracao por vez
gcc -O2 -Wall -Wextra -std=c11 -fopenmp afinidade.c -o afinidade -lm
OMP_PROC_BIND=spread OMP_PLACES=cores ./afinidade spread_cores 2048 200 5 1,4,8,16,32,64,128
```

`TEMPO_BASE` fixa o T(1) usado como base de speedup, para que todas as
configurações sejam comparáveis entre si. `MAPA=1` imprime o mapa `thread → cpu`.

## Arquivos

| Arquivo | Conteúdo |
|---|---|
| `afinidade.c` | simulação, medição de tempo e leitura da topologia ocupada |
| `job_npad.sh` | script Slurm com as doze configurações |
| `resultados.txt` | saída usada no relatório (CSV) |
| `relatorio/graficos.py` | gera os três SVG a partir do `resultados.txt` |
| `relatorio/relatorio.pdf` | relatório com gráficos e código realçado |
| `guia_apresentacao.md` | roteiro para a defesa oral |
