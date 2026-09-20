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

Nó `r2n00`, dois EPYC 7713, 8 domínios NUMA, 32 MB de L3 por CCX de 8 núcleos
(512 MB somados). Malha 2048 = 128 MB nas quatro matrizes.

Escalabilidade forte, tempo em segundos:

| threads | v0 | v1 | v2 | v3 | v4 |
|---:|---:|---:|---:|---:|---:|
| 1 | 3,3060 | 3,2922 | 2,4522 | 2,4399 | 2,4431 |
| 16 | 0,6899 | 0,7336 | 0,6960 | 0,6639 | 0,6593 |
| 32 | 0,1878 | 0,2165 | 0,1879 | 0,1674 | 0,1556 |
| 128 | 0,0761 | 0,0668 | 0,1306 | 0,0583 | **0,0498** |

Eficiência da v0: 1,00 / 0,94 / 0,77 / 0,41 / 0,30 / **0,55** / 0,44 / 0,34 para
1, 2, 4, 8, 16, 32, 64, 128 threads. A subida em 32 threads é real e aparece nas
cinco versões.

Escalabilidade fraca (1024x1024 células por thread): eficiência 1,00 / 0,66 /
0,17 / 0,17 na v0. No maior ponto (64 threads, n=8192) as cinco versões dão o
mesmo tempo, ~4,1 s.

## Gargalos, em ordem de efeito

1. **Capacidade de L3 agregado.** Enquanto o conjunto de trabalho não cabe no L3
   somado das threads, o programa fica preso na DRAM. É o que faz a eficiência
   mergulhar até 16 threads e recuperar em 32, quando 4 CCX somam os 128 MB do
   problema. Banda: 26 GB/s com 8 threads contra 514 GB/s com 128.
2. **Trecho serial nas bordas**, criado na v2. 4,6% de fração serial limitaram o
   speedup a 18,8x. Corrigido na v3: 2,24x de volta.
3. **NUMA / first touch.** 12% em 128 threads, nada abaixo de 32.
4. **Fork-join por passo.** 1,17x na v4.
5. **Banda de memória.** O limite de fundo, que nenhuma versão toca.

## Correção em relação à Tarefa 11

Na Tarefa 11 eu registrei que o gargalo era "first touch / NUMA". Estava
incompleto. O first touch explica só a faixa acima de 32 threads e vale 12%. O
platô de 4 a 16 threads é capacidade de L3 agregado, que é um efeito bem maior e
só apareceu quando a Tarefa 12 mediu ponto a ponto.

## Cuidados de medição que valem explicar

- `mallopt(M_MMAP_THRESHOLD, ...)`: sem isso o glibc reaproveita páginas já
  mapeadas depois de liberar blocos grandes, o first touch não seria refeito
  entre versões e o ganho da v1 viria contaminado.
- Verificação byte a byte antes de medir tempo: a v2 e a v3 mexem justamente no
  tratamento das bordas periódicas.
- `--exclusive` e `OMP_PROC_BIND=close`: sem os dois, a topologia deixa de ser
  previsível e a explicação do L3 agregado não se sustentaria.
