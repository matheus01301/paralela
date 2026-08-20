# Guia para apresentação — Tarefa 2

Este arquivo serve como apoio para a explicação presencial. Ele não faz parte do
relatório entregue em PDF.

## Resumo para apresentação

1. As duas versões calculam a mesma multiplicação `y = A × x`.
2. Na versão por linhas, o laço interno varia a coluna `j`.
3. Na versão por colunas, o laço interno varia a linha `i`.
4. O tempo foi medido com `clock_gettime(CLOCK_MONOTONIC)`, que fornece o tempo de
   parede transcorrido.
5. Para matrizes pequenas, os dados permanecem na cache e os tempos são próximos.
6. A divergência ficou clara em `N = 512`: por colunas levou aproximadamente 6,7
   vezes o tempo por linhas.
7. Em C, a matriz é armazenada por linhas. O acesso por linhas percorre posições
   consecutivas, enquanto o acesso por colunas realiza saltos de `N` elementos.
8. Quando a matriz cresce, o acesso por colunas aproveita pior cada linha de cache
   e precisa buscar dados em níveis mais lentos da memória.

## Perguntas e respostas

### Qual é a diferença entre as duas versões?

Apenas a ordem dos laços. Por linhas, `i` fica no laço externo e `j` no interno.
Por colunas, `j` fica no laço externo e `i` no interno.

### As duas versões fazem o mesmo cálculo?

Sim. Elas multiplicam os mesmos elementos da matriz e do vetor e produzem o mesmo
vetor de resultado. O programa compara os resultados para detectar algum erro.

### Por que os tempos são parecidos para uma matriz pequena?

Porque os dados da matriz pequena cabem nas caches rápidas do processador. Mesmo
que a versão por colunas acesse a memória em uma ordem pior, os dados necessários
continuam próximos do processador e podem ser reutilizados rapidamente.

### A partir de qual tamanho os tempos divergiram significativamente?

Nesta máquina, a divergência ficou clara em `N = 512`. A versão por colunas passou
a levar aproximadamente 6,7 vezes o tempo da versão por linhas.

### Esse tamanho será igual em qualquer computador?

Não. O ponto de divergência depende do tamanho e da organização das caches, do
processador, do compilador e dos tamanhos de matriz escolhidos.

### Por que a versão por linhas é mais rápida?

Em C, a matriz é armazenada por linhas. Variar a coluna no laço interno percorre
posições consecutivas da memória, aproveitando os demais elementos trazidos na
mesma linha de cache.

### Por que a versão por colunas fica mais lenta?

Variar a linha no laço interno faz o programa saltar `N` elementos a cada acesso.
Isso aproveita pior as linhas de cache e, em matrizes grandes, provoca mais acessos
a níveis mais lentos da memória.

### O que é uma linha de cache?

É um bloco de bytes transferido da memória para a cache. Quando o programa solicita
um elemento, outros elementos vizinhos são trazidos junto. O acesso sequencial
aproveita esses dados vizinhos.

### O que significa *wall time*?

É o tempo real transcorrido entre o início e o fim do trecho medido, como o tempo
observado em um relógio na parede.

### Por que foi usado `clock_gettime()`?

Porque a tarefa precisa comparar o tempo real de execução das duas versões. A
função fornece um relógio apropriado e com boa resolução para essa medição.

### Por que foi usado `CLOCK_MONOTONIC`?

Porque esse relógio sempre avança e não é alterado quando a hora do sistema é
corrigida. Assim, uma mudança no relógio do computador não interfere na duração
medida.

### Por que os testes pequenos são repetidos?

Uma única multiplicação pequena termina muito rapidamente e pode ficar próxima da
resolução do cronômetro. Repetir o cálculo e dividir o tempo pela quantidade de
repetições produz uma medida mais estável.

### Para que serve o aquecimento antes da medição?

Ele executa cada versão uma vez para preparar as páginas de memória e reduzir a
influência de custos que só aparecem no primeiro acesso.

### O que representa a coluna “Colunas ÷ linhas”?

Ela divide o tempo da versão por colunas pelo tempo da versão por linhas. Um valor
de `6,72×` significa que a versão por colunas levou 6,72 vezes mais tempo.

### Por que foi usada a opção `-O2`?

Ela solicita otimizações ao compilador. Como a tarefa compara desempenho, todos os
testes precisam usar a mesma configuração de compilação.
