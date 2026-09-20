# Guia de apresentação — Tarefa 12

## Resumo em 30 segundos

Peguei o código da Tarefa 11 e medi a escalabilidade dele num nó do NPAD, em
cinco versões sucessivas. Achei três gargalos: capacidade de L3 agregado, um
trecho serial nas bordas que eu mesmo criei, e NUMA. O código ficou 66× mais
rápido do sequencial original até a última versão em 128 threads, mas **não é
fortemente nem fracamente escalável** — a eficiência termina em 0,38 e 0,09.

## As três definições (o professor vai perguntar)

| termo | definição |
|---|---|
| escalável | mantém a eficiência se o problema crescer junto com as threads |
| **fortemente** escalável | mantém a eficiência **sem** aumentar o problema |
| **fracamente** escalável | mantém a eficiência se o problema crescer na **mesma taxa** das threads |

Fraca ⟹ escalável. Forte ⟹ escalável. Uma **não** implica a outra.

## Números de cor

| | |
|---|---|
| melhor | v4, 128 threads: 0,0498 s — eficiência 0,38 |
| ganho total | 3,306 s → 0,0498 s = **66×** |
| L3 | 32 MB por CCX de 8 núcleos, 512 MB somados; problema = 128 MB |
| fração serial da v2 | 4,6% (Amdahl, a partir do teto de 18,8×) |
| fraca | eficiência cai a 0,09 |

## As partes, na ordem

**1. As cinco versões.** v0 base da Tarefa 11; v1 first touch paralelo; v2
interior sem `%` com bordas em série; v3 bordas distribuídas; v4 uma só região
paralela.

**2. Corretude primeiro.** As 15 combinações (5 versões × 1, 8, 64 threads) deram
campo final idêntico byte a byte à v0 sequencial. Otimizar sem isso não vale nada.

**3. O gráfico de escalabilidade forte.** A eficiência cai até 16 threads, **sobe**
em 32 e volta a cair. As cinco versões fazem o mesmo desenho, então não é ruído.

**4. A explicação.** Com `OMP_PROC_BIND=close`, mais threads ocupam mais CCX, e o
L3 agregado cresce junto. Em 32 threads são 4 CCX = 128 MB, que é exatamente o
tamanho do problema. Os dados passam a sobreviver entre passos. Prova pela banda:
26 GB/s em 8 threads (DRAM) contra 514 GB/s em 128 (só pode ser cache).

**5. A v2, que piorou.** Tirar o `%` deixou o sequencial 1,35× mais rápido e o
paralelo 1,7× mais lento. Eu tinha deixado as bordas em série: colunas 0 e n−1 de
2046 linhas, acessos dispersos, uma thread só. A v3 distribui e devolve 2,24×.

**6. Escalabilidade fraca.** No maior ponto (64 threads, n=8192) as cinco versões
dão o mesmo tempo, ~4,1 s. 2 GB de matrizes contra 512 MB de L3: não cabe mais
nada, vira banda de memória pura, e nenhuma otimização toca nesse limite.

## Se perguntarem

**O programa é escalável?** Não fortemente (eficiência 1,00 → 0,38) e não
fracamente (→ 0,09). Rigorosamente, falhar no teste fraco não prova que não é
escalável em *nenhuma* taxa — só naquela. Mas como o limite é banda de memória,
que não cresce com `n`, crescer mais rápido pioraria.

**Por que a eficiência sobe de 16 para 32 threads?** Porque o L3 agregado passa a
caber o problema. É ganho superlinear: o tempo cai 3,7× dobrando as threads.

**Por que o first touch só ajuda acima de 32 threads?** Abaixo disso todas as
threads estão no mesmo nó NUMA, então não há para onde distribuir as páginas.
Vale 12% em 128 threads.

**Como você chegou nos 4,6% de fração serial?** Por Amdahl, invertendo: speedup
preso em 18,8× com 128 threads só é possível com ~4,6% de código serial.

**Por que as versões otimizadas têm eficiência fraca *pior*?** Porque o T(1)
delas é 1,85× menor. O denominador mudou. Eficiência entre versões de bases
diferentes engana — a comparação honesta é o tempo absoluto, e por ele a v3 é a
melhor no maior ponto.

**Por que `mallopt` no começo do código?** O glibc aumenta o limite de `mmap`
depois de liberar blocos grandes e passa a reaproveitar páginas já mapeadas. Se
isso acontecesse, o first touch não seria refeito entre versões e o ganho da v1
apareceria contaminado pela v0.

**Na v4, como as threads trocam os ponteiros sem se atropelar?** Os quatro
ponteiros entram como `firstprivate`, então cada thread troca a própria cópia. A
barreira implícita do `omp for` garante que ninguém comece o passo seguinte antes
de todos terminarem.

## Comandos

```bash
ssh npad
cd ~/paralela/tarefa-12
sbatch job_npad.sh

cd tarefa-12/relatorio
python graficos.py && python build_pdf.py
```
