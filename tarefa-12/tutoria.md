# Tutoria - Tarefa 12

## Etapa atual

Atividade concluída: código com cinco versões, medições no NPAD, três gráficos,
relatório em PDF e guia de defesa. Feita em ritmo de entrega, sem as paradas de
tutoria, como combinado para as Tarefas 11 a 13.

**Para estudar depois:** o `guia_apresentacao.md` tem tudo. O que mais
provavelmente cai são as três definições de escalabilidade e a explicação do
salto de eficiência entre 16 e 32 threads.

## O que a atividade pede

- Avaliar a escalabilidade do código de Navier-Stokes num nó do NPAD.
- Identificar gargalos de escalabilidade.
- Reportar o progresso em versões sucessivas do código otimizado.
- Comentar escalabilidade, escalabilidade fraca e escalabilidade forte.
- Relatório em PDF com o código realçado.

Diferença importante em relação à Tarefa 11: lá o enunciado mandava *explorar* as
cláusulas e otimizar era errado. Aqui otimizar é exatamente o que se pede.

## Definições usadas (Pacheco)

- **Escalável**: existe uma taxa de crescimento do problema que mantém a
  eficiência constante conforme as threads aumentam.
- **Fortemente escalável**: mantém a eficiência aumentando as threads *sem*
  aumentar o problema.
- **Fracamente escalável**: mantém a eficiência se o problema crescer na *mesma
  taxa* das threads.

Fraca implica escalável; forte implica escalável; uma não implica a outra.

## Resultados principais

Nó `r2n19`, dois EPYC 7713, 8 domínios NUMA, 32 MB de L3 por CCX de 8 núcleos
(512 MB somados). Malha 2048 = 128 MB nas quatro matrizes.

Escalabilidade forte, tempo em segundos:

| threads | v0 | v1 | v2 | v3 | v4 |
|---:|---:|---:|---:|---:|---:|
| 1 | 4,4549 | 4,4659 | 3,5188 | 3,5310 | 3,5507 |
| 16 | 0,6798 | 0,6841 | 0,6628 | 0,6420 | 0,6233 |
| 32 | 0,2010 | 0,1690 | 0,2110 | 0,1519 | 0,1472 |
| 128 | 0,0763 | 0,0656 | 0,1236 | 0,0564 | **0,0494** |

Eficiência da v0: 1,00 / 0,98 / 0,93 / 0,55 / 0,41 / **0,69** / 0,61 / 0,46 para
1, 2, 4, 8, 16, 32, 64, 128 threads. A subida em 32 threads é real e aparece nas
cinco versões.

Escalabilidade fraca (1024x1024 células por thread): eficiência 1,00 / 0,71 /
0,21 / 0,20 na v0. No maior ponto (64 threads, n=8192) as cinco versões dão o
mesmo tempo, ~4,1 s.

## Gargalos, em ordem de efeito

1. **Capacidade de L3 agregado.** Enquanto o conjunto de trabalho não cabe no L3
   somado das threads, o programa fica preso na DRAM. É o que faz a eficiência
   mergulhar até 16 threads e recuperar em 32, quando 4 CCX somam os 128 MB do
   problema. Banda: 25 GB/s com 8 threads contra 518 GB/s com 128.
2. **Trecho serial nas bordas**, criado na v2. 2,8% de fração serial limitaram o
   speedup a 28,5x. Corrigido na v3: 2,19x de volta.
3. **NUMA / first touch.** 14% em 128 threads, nada abaixo de 32.
4. **Fork-join por passo.** 1,14x na v4.
5. **Banda de memória.** O limite de fundo, que nenhuma versão toca.

## Correção em relação à Tarefa 11

Na Tarefa 11 eu registrei que o gargalo era "first touch / NUMA". Estava
incompleto. O first touch explica só a faixa acima de 32 threads e vale 14%. O
platô de 4 a 16 threads é capacidade de L3 agregado, que é um efeito bem maior e
só apareceu quando a Tarefa 12 mediu ponto a ponto.

## Sobre o nó usado

As medições foram refeitas no `r2n19` porque a Tarefa 13 exige o mesmo nó da 12 e
o `r2n00`, usado na primeira rodada, ficou reservado por outro job até 10/10. Os
nós `amd-512` são idênticos (mesmo EPYC 7713, mesma topologia), e todas as
conclusões se mantiveram; apenas os números absolutos mudaram.

## Cuidados de medição que valem explicar

- `mallopt(M_MMAP_THRESHOLD, ...)`: sem isso o glibc reaproveita páginas já
  mapeadas depois de liberar blocos grandes, o first touch não seria refeito
  entre versões e o ganho da v1 viria contaminado.
- Verificação byte a byte antes de medir tempo: a v2 e a v3 mexem justamente no
  tratamento das bordas periódicas.
- `--exclusive` e `OMP_PROC_BIND=close`: sem os dois, a topologia deixa de ser
  previsível e a explicação do L3 agregado não se sustentaria.
