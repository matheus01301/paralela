# Tarefa 4 — OpenMP: limites de memória e de CPU

Esta tarefa contém dois programas paralelos em C. `memory_bound.c` soma vetores
grandes e é limitado principalmente pela largura de banda da memória. O programa
`cpu_bound.c` executa muitas recorrências aritméticas sobre poucos dados e é
limitado principalmente pelas unidades de cálculo do processador.

Ambos usam `#pragma omp parallel for`, testam de 1 até o número de processadores
lógicos disponível e apresentam tempo, *speedup* e eficiência. Cada resultado é
a mediana de cinco amostras, depois de um aquecimento. A intensidade aritmética e
o desempenho em GFLOP/s alimentam um Roofline Model empírico dos dois kernels.

## Compilar e executar

```bash
gcc -O2 -Wall -Wextra -std=c99 -fopenmp memory_bound.c -o memory_bound.exe
gcc -O2 -Wall -Wextra -std=c99 -fopenmp cpu_bound.c -o cpu_bound.exe -lm

./memory_bound.exe
./cpu_bound.exe
```

Os argumentos opcionais permitem mudar a carga:

```bash
./memory_bound.exe ELEMENTOS REPETICOES
./cpu_bound.exe ELEMENTOS ITERACOES_INTERNAS
```

No PowerShell, o script abaixo compila, executa e atualiza os arquivos de
resultados:

```powershell
.\run_benchmarks.ps1
```

## Resultado observado

No Intel Core i7-13650HX usado no teste, o programa de memória passou de 20,96
GB/s com uma thread para 50,36 GB/s com 20 threads. A maior parte do ganho já
ocorreu até quatro threads; depois disso, a largura de banda aproximou-se do teto.

No programa de CPU, a mediana consolidada mostrou melhora até 20 threads, com
0,04211 s e *speedup* de 12,663×. Como não foi fixada afinidade, esse resultado
representa o comportamento conjunto do OpenMP e do escalonador do Windows nesta
CPU híbrida; ele não isola o efeito do Hyper-Threading.

No Roofline, a soma possui intensidade de apenas `0,0417 FLOP/byte` e fica sob o
teto de memória. As recorrências possuem `200 FLOP/byte` e ficam sob o teto de
processamento. Com os tetos sustentados medidos, o ponto de equilíbrio é `1,51
FLOP/byte`.

## Arquivos

| arquivo | conteúdo |
|---|---|
| `memory_bound.c` | soma de vetores, limitada pela memória |
| `cpu_bound.c` | recorrências aritméticas, limitadas pela CPU |
| `run_benchmarks.ps1` | compilação e execução automatizadas |
| `resultados_memory.txt` | saída medida do teste de memória |
| `resultados_cpu.txt` | saída medida do teste de CPU |
| `roofline.svg` | gráfico do Roofline Model empírico |
| `relatorio/relatorio.pdf` | relatório pronto para entrega |
| `guia_apresentacao.md` | resumo e perguntas para apresentação |

Os resultados dependem do processador, do estado térmico, do sistema operacional
e do posicionamento das threads. Por isso, devem ser medidos novamente quando o
experimento for executado em outra máquina.
