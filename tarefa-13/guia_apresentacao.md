# Guia de apresentação — Tarefa 13

## Resumo em 30 segundos

Rodei o meu código de Navier-Stokes (o da 11, com `static`) no mesmo nó da 12
(`r2n19`), mudando só a afinidade: `false`, `true`, `close`, `spread` e `master` do
OpenMP, e `taskset` e `numactl` do sistema. Cada thread diz em qual CPU ficou. Com
8 e 16 threads, `spread` foi 1,5–1,7× melhor que `close`; com muitas, `close` ganhou.
`master` pôs tudo num núcleo: 26× mais lento que 1 thread.

## Números de cor

| | |
|---|---|
| máquina | 128 núcleos, 8 nós NUMA de 16, L3 de 32 MB a cada 8 núcleos |
| CPUs 128–255 | segunda thread de hardware dos núcleos 0–127 |
| 16 threads | `spread` 0,348 s, `close` 0,591 s, `master` 11,3 s |
| 128 threads | `close` 0,058 s, `spread` 0,081 s, `false` 0,254 s, `master` 103,9 s |
| `numactl --interleave`, 16 threads | 0,418 s, 1,4× melhor que `close` |

## Se perguntarem

**O que é afinidade?** Prender cada thread a um lugar da máquina, para o sistema não
ficar mudando ela de núcleo.

**Diferença entre close e spread?** `close` põe as threads em núcleos vizinhos.
`spread` espalha pela máquina. Com 8 threads: `close` usou os núcleos 0–7, `spread`
usou 0, 16, 32… (um por nó NUMA).

**Por que spread ganhou com poucas threads?** Cada thread ficou num nó NUMA
diferente: mais cache e mais ligações com a memória. No `close`, as 8 dividem um
cache de 32 MB e a memória de um nó só. O código é limitado por memória (Tarefa 12).

**E por que close ganhou com muitas?** Aí todos já estão espalhados. Provável: com
`close`, threads vizinhas calculam linhas vizinhas e dividem o cache na fronteira.
(Não medi diretamente.)

**O que o `OMP_PLACES=cores` faz?** Define que cada "lugar" é um núcleo físico. O
`OMP_PROC_BIND` decide como as threads ocupam esses lugares.

**E o `true`?** Prende as threads sem dizer como espalhar. Aqui ficou igual ao
`close` (núcleos 0–7).

**Por que false é ruim?** O sistema decide e pode mudar as threads de núcleo. Às
vezes espalha bem (8 e 16 threads), às vezes empilha (128 threads: 4,4× pior).

**Por que master foi tão ruim?** Todas as threads no núcleo 0, revezando. Mais
threads, mais fila.

**Como você sabe onde as threads ficaram?** Cada thread chama `sched_getcpu()`, que
devolve a CPU em que ela está, e o programa imprime.

**Afinidade pelo sistema operacional?** `taskset -c 0-15` limita o programa às CPUs
0–15. `numactl --interleave=all` espalha as páginas de memória por todos os nós
NUMA: ajudou com 16 threads, como um "spread da memória".

**Por que a inicialização é paralela?** First touch: a memória fica no nó NUMA de
quem escreve primeiro. Com a mesma divisão do cálculo, cada thread deixa os seus
dados perto de si. Assim a afinidade das threads leva a memória junto.

**Taxonomia de Flynn?** SISD (1 instrução, 1 dado), SIMD (1 instrução, vários dados:
vetorização, GPU), MISD (raro), MIMD (várias instruções, vários dados: multicore,
cluster). O nó do NPAD é MIMD de memória compartilhada, do tipo NUMA.

**UMA × NUMA?** UMA: todos os núcleos acessam a memória com o mesmo tempo. NUMA:
cada região de memória está mais perto de alguns núcleos; acesso longe é mais lento.

**Por que o mesmo nó da 12?** O enunciado pede, para comparar com a 12 no mesmo
hardware.

## Detalhes do código

**`#define _GNU_SOURCE`?** Libera o `sched_getcpu`, que é uma extensão do Linux.

**`sched_getcpu()`?** Devolve o número da CPU em que a thread está naquele momento.

**`cpu[omp_get_thread_num()] = sched_getcpu()` tem condição de corrida?** Não: cada
thread escreve na sua própria posição do vetor.

**Por que no `close` a thread 0 aparece na CPU 128, e não na 0?** A CPU 128 é a
segunda thread de hardware do núcleo 0. Com `OMP_PLACES=cores`, o lugar é o núcleo
inteiro, e a thread pode rodar em qualquer uma das duas.

**`fflush(stdout)`?** Força a linha a aparecer no arquivo na hora, sem esperar o
fim do programa.

**Por que o menor de 5 tempos?** O menor é o que sofreu menos interferência.

**Por que o `taskset` só vai até 16 threads?** Ele limita o programa a 16 CPUs. Com
mais threads, elas dividiriam as mesmas CPUs.

## Comandos

```bash
ssh npad
cd ~/paralela/tarefa-13
sbatch job_npad.sh

cd tarefa-13/relatorio
python graficos.py && python build_pdf.py
```
