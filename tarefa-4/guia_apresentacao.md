# Guia para apresentação — Tarefa 4

Este arquivo serve como apoio para a explicação presencial. Ele não faz parte do
relatório entregue em PDF.

## Resumo para apresentação

1. Os dois programas usam `#pragma omp parallel for schedule(static)`.
2. O programa *memory-bound* faz uma soma por elemento, mas transfere pelo menos
   24 bytes; seus três vetores ocupam 768 MiB e não cabem na cache.
3. O programa *compute-bound* reutiliza cada valor em 200 iterações de recorrências
   aritméticas; os dados ocupam apenas 15,3 MiB.
4. Foi medido tempo de parede, com aquecimento e mediana de cinco amostras.
5. O Roofline usa `P(I) = min(Pmáx, Bmáx × I)` e separa os limites de memória e CPU.
6. A intensidade da soma é 0,0417 FLOP/byte; a das recorrências é 200 FLOP/byte.
7. Na soma de vetores, o ganho foi rápido até quatro threads e depois estabilizou
   perto de 50 GB/s. O melhor resultado foi 2,403× com 20 threads.
8. O programa de CPU continuou melhorando até 20 threads, quando atingiu 12,663×.
9. Não foi fixada afinidade, portanto o teste mede o conjunto OpenMP + escalonador
   do Windows e não isola o efeito do Hyper-Threading.
10. Conceitualmente, threads de hardware podem esconder espera de memória, mas
    também podem competir pelas unidades aritméticas em uma carga de CPU.

## Perguntas e respostas

### O que significa *memory-bound*?

Significa que o desempenho é limitado principalmente pela velocidade com que os
dados chegam da memória, e não pela capacidade de fazer somas. A CPU termina a
operação rapidamente e passa parte do tempo esperando dados.

### Por que a soma de vetores é limitada por memória?

Para cada elemento ela faz somente uma soma, mas lê dois `double` e grava outro.
Isso representa pelo menos 24 bytes movimentados para uma operação. Além disso,
os 768 MiB dos vetores são muito maiores que a cache compartilhada.

### O que significa *compute-bound*?

Significa que as unidades de cálculo da CPU são o principal limite. O programa
realiza muitas multiplicações e somas para cada pequena quantidade de dados lida.

### O que é o Roofline Model?

É um modelo que compara intensidade aritmética e desempenho. O limite previsto é
o menor entre o pico computacional e a largura de banda multiplicada pela
intensidade: `P(I) = min(Pmáx, Bmáx × I)`.

### O que é intensidade aritmética?

É a quantidade de operações de ponto flutuante realizada para cada byte movido.
Uma intensidade baixa tende ao limite de memória; uma intensidade alta tende ao
limite de processamento.

### Onde os dois programas ficam no Roofline?

A soma fica na região *memory-bound*, com 0,0417 FLOP/byte e 2,10 GFLOP/s. As
recorrências ficam na região *compute-bound*, com 200 FLOP/byte e até 76,00
GFLOP/s. O ponto de equilíbrio medido foi 1,51 FLOP/byte.

### O gráfico representa o pico teórico do processador?

Não. Ele usa tetos sustentados obtidos pelos próprios kernels: 50,36 GB/s e 76,00
GFLOP/s. Portanto, é um Roofline empírico adequado para comparar estes programas,
não uma especificação do pico absoluto da microarquitetura.

### Como os laços foram paralelizados?

Com `#pragma omp parallel for schedule(static)`. Cada índice é independente e o
OpenMP divide blocos do vetor entre as threads.

### Por que usar `schedule(static)`?

Todas as iterações têm praticamente o mesmo custo. Uma divisão estática equilibra
o trabalho sem acrescentar o custo de distribuir novas iterações durante o laço.

### Como o tempo foi medido?

Com tempo de parede de alta resolução: `QueryPerformanceCounter` no Windows e
`clock_gettime(CLOCK_MONOTONIC)` em POSIX. Cada configuração foi aquecida e o
resultado é a mediana de cinco amostras. O teste de CPU foi executado três vezes,
e a tabela utiliza outra mediana entre essas execuções completas.

### O que é *speedup*?

É o tempo com uma thread dividido pelo tempo com `p` threads. Por exemplo, 2,403×
indica que a execução paralela foi 2,403 vezes mais rápida que a execução com uma
thread.

### O que é eficiência?

É o *speedup* dividido pelo número de threads. Ela mostra quanto do crescimento
ideal foi aproveitado. A eficiência tende a cair por gargalos compartilhados,
overhead e partes que não aceleram proporcionalmente.

### Quando o programa de memória melhorou e quando estabilizou?

O maior ganho ocorreu de uma a quatro threads: 20,96 para 42,46 GB/s. A partir daí
os ganhos ficaram menores. Entre 10 e 20 threads, o resultado permaneceu perto de
47–50 GB/s, indicando saturação da largura de banda.

### Por que threads lógicas podem ajudar uma carga de memória?

Quando uma thread espera dados, outra thread lógica do mesmo núcleo pode executar
instruções e manter outras requisições em andamento. Isso esconde parte da
latência, desde que ainda exista largura de banda disponível.

### Por que podem atrapalhar uma carga de CPU?

Duas threads lógicas no mesmo núcleo não recebem duas CPUs completas. Elas
compartilham unidades aritméticas, filas e cache. Se uma thread já utiliza esses
recursos intensamente, a segunda cria competição e pode aumentar o tempo.

### O experimento mediu isoladamente o efeito do Hyper-Threading?

Não. O OpenMP criou as threads e o Windows decidiu onde executá-las. O resultado
com 20 threads foi o melhor da carga de CPU, mas não podemos separar nele os
efeitos de P-cores, E-cores, SMT e escalonamento. A tarefa pede apenas a reflexão
conceitual sobre esses efeitos, não um experimento de afinidade.

### Por que 20 threads não são 20 núcleos?

O processador tem 14 núcleos físicos e 20 processadores lógicos. Seus seis P-cores
aceitam duas threads de hardware; os oito E-cores aceitam uma. Assim, parte das 20
threads precisa compartilhar um núcleo físico.

### O melhor número de threads será igual em outra máquina?

Não. Ele depende da quantidade e do tipo dos núcleos, largura de banda, cache,
frequência, temperatura, compilador, afinidade e carga do sistema. O correto é
repetir a medição na máquina de interesse.

### Por que foi usada a mediana em vez da média?

Uma interrupção do sistema pode produzir uma amostra excepcionalmente lenta e
puxar a média. A mediana escolhe o valor central e é menos sensível a esse ruído.

### Por que foi usado `-O2`?

Porque a tarefa analisa desempenho real. `-O2` aplica otimizações normais do
compilador, e a mesma opção é usada nos dois programas para manter a comparação
consistente.
