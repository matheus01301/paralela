# Tarefa 12 — escalabilidade do Navier-Stokes no NPAD

Avaliação de escalabilidade **forte** e **fraca** do código da Tarefa 11, com os
gargalos identificados e removidos em cinco versões sucessivas.

| versão | mudança | gargalo atacado |
|---|---|---|
| `v0` | código da Tarefa 11 | referência |
| `v1` | inicialização paralela com o mesmo `schedule(static)` do cálculo | *first touch* / NUMA |
| `v2` | interior sem `%`, bordas em série | divisões inteiras por célula |
| `v3` | cada thread fecha as bordas das próprias linhas | trecho serial criado pela `v2` |
| `v4` | uma única região paralela em volta do laço de tempo | fork-join por passo |

As cinco versões são verificadas byte a byte contra a `v0` sequencial em 1, 8 e
64 threads antes de qualquer medição de tempo.

**Resultado:** 66× do sequencial original até a `v4` em 128 threads, mas o
programa **não é fortemente nem fracamente escalável** — eficiência final de 0,38
e 0,09. O gargalo dominante é capacidade de L3 agregado, e no limite, banda de
memória.

## Executar

```bash
sbatch job_npad.sh          # no NPAD: verificação + escalabilidade forte e fraca
cat slurm-<JobID>.out

# direto
gcc -O2 -Wall -Wextra -std=c11 -fopenmp escalabilidade.c -o escalabilidade -lm
./escalabilidade verificar 256 40
./escalabilidade forte 2048 200 5 128     # modo, n, passos, execucoes, max threads
./escalabilidade fraca 1024 200 5 64      # n cresce com p, trabalho/thread constante
```

## Arquivos

| Arquivo | Conteúdo |
|---|---|
| `escalabilidade.c` | as cinco versões, verificação e os dois experimentos |
| `job_npad.sh` | script Slurm (nó exclusivo, 128 núcleos, threads fixadas) |
| `diagnostico.sh` | levanta topologia do nó: NUMA, caches e *places* do OpenMP |
| `resultados.txt` | saída usada no relatório (CSV) |
| `relatorio/graficos.py` | gera os três SVG a partir do `resultados.txt` |
| `relatorio/relatorio.pdf` | relatório com gráficos e código realçado |
| `guia_apresentacao.md` | roteiro para a defesa oral |
