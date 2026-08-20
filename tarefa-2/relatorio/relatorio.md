# Multiplicação matriz-vetor e o efeito da memória cache

## 1. Objetivo

O objetivo desta tarefa foi implementar duas versões da multiplicação de uma
matriz por um vetor e comparar seus tempos de execução. As versões realizam o
mesmo cálculo, mas percorrem a matriz em ordens diferentes: por linhas e por
colunas.

## 2. Implementação

Para uma matriz quadrada `A`, um vetor `x` e um vetor de resultado `y`, cada
posição do resultado é calculada por:

<div class="formula">y[i] = A[i][0] · x[0] + A[i][1] · x[1] + ⋯ + A[i][N−1] · x[N−1]</div>

| Versão | Laço externo | Laço interno | Ordem percorrida |
|---|---|---|---|
| Por linhas | linha `i` | coluna `j` | elementos consecutivos da linha |
| Por colunas | coluna `j` | linha `i` | elementos da mesma coluna |

As duas versões foram executadas sobre os mesmos dados. O programa também compara
os vetores produzidos para verificar se ambos os métodos chegaram ao mesmo
resultado.

## 3. Medição do tempo

Foi utilizado o tempo de parede (*wall time*) por meio de
`clock_gettime(CLOCK_MONOTONIC)`. Esse relógio mede o tempo transcorrido e não é
afetado por ajustes no relógio do sistema.

Os testes menores são repetidos várias vezes e o tempo total é dividido pela
quantidade de repetições. Antes da medição, cada versão é executada uma vez como
aquecimento. Isso reduz a influência da resolução do cronômetro e da primeira
alocação das páginas de memória.

O programa foi compilado com `gcc -O2 -Wall -Wextra -std=c99`. Os resultados são
a mediana de três execuções em um Intel Core i7-13650HX com Windows 11. O tempo
pode variar entre computadores e entre execuções.

## 4. Resultados

| N | Matriz (MB) | Por linhas (s) | Por colunas (s) | Colunas ÷ linhas |
|---:|---:|---:|---:|---:|
| 64 | 0,03 | 0,000002 | 0,000002 | 0,98× |
| 128 | 0,13 | 0,000010 | 0,000018 | 1,82× |
| 256 | 0,50 | 0,000046 | 0,000103 | 2,25× |
| **512** | **2,00** | **0,000193** | **0,001298** | **6,72×** |
| 1024 | 8,00 | 0,000793 | 0,005305 | 6,76× |
| 2048 | 32,00 | 0,003330 | 0,023834 | 7,11× |
| 4096 | 128,00 | 0,014773 | 0,107294 | 7,25× |

Para `N = 64`, os tempos foram praticamente iguais. Em `N = 128` e `N = 256`, a
versão por colunas já apresentou alguma diferença, mas os tempos absolutos ainda
eram muito pequenos. A divergência se tornou significativa em **N = 512**, quando
o acesso por colunas passou a levar aproximadamente **6,7 vezes** o tempo do acesso
por linhas. Nos tamanhos seguintes, a diferença permaneceu próxima ou superior a
esse valor.

## 5. Explicação pela memória cache

Em C, os elementos de uma matriz são armazenados por linhas. Por isso,
`matriz[i * n + j]` e `matriz[i * n + j + 1]` ocupam posições vizinhas na memória.

O processador não busca apenas um `double` por vez na memória principal. Ele traz
um bloco chamado **linha de cache**, que contém vários elementos consecutivos. Na
versão por linhas, esses elementos são usados logo nas próximas iterações. Esse
padrão sequencial também é fácil de antecipar pelo processador.

Na versão por colunas, cada incremento de `i` salta aproximadamente `N` elementos
na memória. Assim, o processador traz uma linha de cache, utiliza apenas uma parte
dela naquele momento e salta para uma região distante. Quando a matriz é pequena,
grande parte dos dados permanece na cache e a diferença é pequena. Conforme a
matriz cresce, os dados deixam de caber nas caches mais rápidas e o acesso por
colunas provoca mais buscas em níveis mais lentos da memória.

O processador utilizado possui 24 MB de cache compartilhada. Em `N = 2048`, a
matriz ocupa 32 MB e já ultrapassa essa capacidade. Isso ajuda a explicar por que
a diferença permanece elevada nos maiores testes. O ponto exato da divergência
não é universal: ele depende dos tamanhos das caches, do processador, do compilador
e das dimensões testadas.

## 6. Conclusão

As duas versões executam a mesma multiplicação e produzem o mesmo resultado, mas
possuem tempos diferentes devido ao padrão de acesso à memória. Nesta máquina, a
divergência ficou clara a partir de `N = 512`. O acesso por linhas foi mais rápido
porque percorreu endereços consecutivos e aproveitou melhor as linhas de cache. O
experimento mostra que o desempenho não depende apenas da quantidade de operações:
a ordem em que os dados são acessados também é importante.

<div class="quebra"></div>

## 7. Código-fonte

{{CODIGO:mxv.c}}
