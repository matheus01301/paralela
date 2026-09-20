# Guia de apresentação — Tarefa 13

## Resumo em 30 segundos

Peguei o melhor código da Tarefa 12 e variei só a afinidade das threads, no mesmo
nó. `spread` ficou **2,4× mais rápido** que `close` em 16 threads, e o platô de
escalabilidade que eu tinha achado na Tarefa 12 **desapareceu**. Ou seja: aquele
platô não era do código, era da afinidade que eu havia fixado.

## Números de cor

| | |
|---|---|
| base comum | T(1) = 3,1714 s, mediana de 9 |
| `spread` vs `close` | 2,0× em 8 threads, **2,4× em 16** |
| `master` | 52,5 s em 128 threads = **16,6× mais lento que o sequencial** |
| melhor em 64 threads | `numactl --interleave` + `close`: 0,076 s |
| melhor em 128 threads | `spread`/`threads`: 0,050 s |
| ruído | ±17% no T(1) — não interpretar diferenças menores que ~20% |

## As partes, na ordem

**1. O que muda e o que não muda.** O código é o mesmo da Tarefa 12 (v4). Só a
afinidade varia.

**2. Os dois eixos do OpenMP.** `OMP_PROC_BIND` = política (`false`, `master`,
`close`, `spread`); `OMP_PLACES` = o que é um lugar (`threads`, `cores`,
`sockets`).

**3. O mapa medido com 8 threads.** `close`/`cores` → CPUs 128,1..7, todas no nó
NUMA 0, **1 domínio de L3**. `spread`/`cores` → 128,16,32,48,64,80,96,112, **uma
por nó NUMA, 8 domínios de L3**. Não é suposição: o programa lê `sched_getcpu()` e
cruza com `/sys`.

**4. O resultado.** Eficiência com `close`: 0,79 → 0,42 → 0,34 (4, 8, 16 threads).
Com `spread`: 0,88 → 0,85 → 0,80.

**5. Por que.** Problema = 128 MB; cada CCX tem 32 MB de L3. `close` com 8 threads
dá 32 MB; `spread` dá 8 × 32 = 256 MB e o problema passa a caber. É a mesma
explicação da Tarefa 12, confirmada mudando outra variável.

**6. O lado do sistema operacional.** `numactl --interleave=all` melhora o `close`
em 1,40× com 16 threads, sem tocar no código.

## Se perguntarem

**Qual a diferença entre `close` e `spread`?** `close` preenche lugares
consecutivos a partir do lugar da thread mestre. `spread` distribui as threads o
mais longe possível umas das outras.

**Quando usar cada uma?** `spread` para código limitado por memória, que ganha
cache e controladores de memória. `close` para código limitado por CPU que
compartilha dados entre threads vizinhas, porque mantém elas perto no cache.

**O que é `OMP_PLACES=threads` e por que atrapalhou?** Lugar passa a ser cada
thread de hardware, então as irmãs SMT contam separado. Com `close`, 8 threads
ocupam só 4 núcleos físicos, duas por núcleo, disputando as mesmas unidades e o
mesmo L1/L2. Com `spread` não atrapalha, porque as irmãs ficam longe.

**Por que `master` é tão ruim?** Prende todas as threads no lugar da mestre — 128
threads em 2 CPUs. 52,5 s contra 3,17 s do sequencial.

**Testou afinidade por domínio NUMA no OpenMP?** Tentei `OMP_PLACES=numa_domains`,
mas é do OpenMP 5.0 e o gcc 8.5 implementa o 4.5. O libgomp responde `Invalid
value for environment variable OMP_PLACES` e ignora os lugares. Por isso o
controle por NUMA teve que vir do sistema, com `numactl`.

**O que foi aquela configuração `GOMP` que explodiu?** `GOMP_CPU_AFFINITY=0-127:16`
tem passo 16, então a lista só tem 8 CPUs. Até 8 threads é ótima. De 16 em diante
ela dá a volta e várias threads dividem a mesma CPU: 24 s com 128 threads. E nada
avisa — só percebi porque meço quantas CPUs distintas foram ocupadas.

**Como você sabe que não é ruído?** Medi ±17% de variação no T(1) entre
invocações. Por isso não concluo nada abaixo de ~20% — inclusive descarto a
diferença entre `close` e `spread` com 4 threads, que é de 12%. Os efeitos
centrais (2,4×, 16,6×) estão muito acima disso.

**Isso contradiz a Tarefa 12?** Não, confirma. Lá eu disse que o gargalo era L3
agregado, com a afinidade fixa. Aqui mudo só a afinidade e o efeito aparece
exatamente onde a explicação previa. Como controle, `close`/`cores` aqui e a `v4`
de lá deram tempos com 5% a 6% de diferença.

## Comandos

```bash
ssh npad
cd ~/paralela/tarefa-13
sbatch job_npad.sh

cd tarefa-13/relatorio
python graficos.py && python build_pdf.py
```
