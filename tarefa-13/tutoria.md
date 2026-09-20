# Tutoria - Tarefa 13

## Etapa atual

Atividade concluída: código, medições no NPAD, três gráficos, relatório em PDF e
guia de defesa. Ritmo de entrega, sem as paradas de tutoria, como combinado para
as Tarefas 11 a 13.

**Para estudar depois:** o `guia_apresentacao.md`. O que mais provavelmente cai é
a diferença entre `close` e `spread`, e por que `OMP_PLACES=threads` atrapalha.

## O que a atividade pede

- Avaliar como a escalabilidade muda com os tipos de afinidade suportados pelo
  sistema operacional e pelo OpenMP.
- No mesmo nó de computação usado na Tarefa 12.
- Relatório em PDF com o código realçado.

## Por que a Tarefa 12 foi refeita

O enunciado exige o mesmo nó. O `r2n00`, usado na primeira rodada da Tarefa 12,
ficou reservado por outro job até 10/10/2026. As duas tarefas foram então rodadas
no `r2n19`, que é idêntico, e os números da Tarefa 12 foram atualizados. Nenhuma
conclusão dela mudou.

## Resultado principal

`spread` contra `close`, com `OMP_PLACES=cores`, tempo em segundos:

| threads | close | spread | razao | dominios de L3 (close -> spread) |
|---:|---:|---:|---:|---:|
| 4 | 1,009 | 0,903 | 1,12x | 1 -> 4 |
| 8 | 0,940 | 0,468 | **2,01x** | 1 -> 8 |
| 16 | 0,586 | 0,248 | **2,36x** | 2 -> 16 |
| 32 | 0,150 | 0,145 | 1,03x | 4 -> 16 |
| 128 | 0,054 | 0,053 | 1,03x | 16 -> 16 |

O ganho de 4 threads (12%) fica abaixo do ruido medido e nao deve ser afirmado.

## A ligacao com a Tarefa 12

O platô de eficiência entre 4 e 16 threads que a Tarefa 12 encontrou nao era
propriedade do codigo: era consequencia de `OMP_PROC_BIND=close`, que eu havia
fixado. Com `spread` ele desaparece. Isso **confirma** a explicacao da Tarefa 12
por um caminho independente: o gargalo e capacidade de L3 agregado, e a afinidade
decide quanto L3 o programa alcanca.

Controle de reprodutibilidade: `close`/`cores` aqui e a `v4` da Tarefa 12 sao o
mesmo codigo com a mesma afinidade, em jobs diferentes. Deram 5% a 6% de
diferenca.

## Achados que valem para a defesa

- `master` prende tudo em 2 CPUs: 52,5 s com 128 threads, 16,6x mais lento que o
  sequencial.
- `OMP_PLACES=threads` com `close` ocupa irmas SMT do mesmo nucleo: 0,893 s contra
  0,586 s de `cores`, em 16 threads. Com `spread` nao atrapalha.
- `OMP_PLACES=numa_domains` e do OpenMP 5.0 e o gcc 8.5 implementa o 4.5. O
  libgomp recusa e ignora os lugares.
- `numactl --interleave=all` melhora o `close` em 1,40x com 16 threads sem tocar
  no codigo, e da o melhor tempo geral em 64 threads.
- `GOMP_CPU_AFFINITY=0-127:16` tem passo 16, logo so 8 CPUs na lista. A partir de
  16 threads ela da a volta e threads dividem CPU: 24 s com 128 threads. So foi
  percebido porque o programa mede quantas CPUs distintas foram ocupadas.

## Cuidado de medicao

Os treze valores de T(1) variaram entre 2,49 s e 3,52 s, ou +-17%, mesmo cada um
sendo mediana de 5 execucoes num no exclusivo. Por isso o relatorio usa uma base
unica de speedup (`TEMPO_BASE`) e nao conclui nada abaixo de ~20% de diferenca.
