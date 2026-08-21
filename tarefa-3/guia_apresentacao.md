# Guia para apresentação — Tarefa 3

Este arquivo serve como apoio para a explicação presencial. Ele não faz parte do
relatório entregue em PDF.

## Resumo para apresentação

1. ILP é a capacidade de executar instruções independentes ao mesmo tempo dentro
   de um único núcleo.
2. A inicialização possui iterações independentes, pois cada uma escreve em uma
   posição diferente do vetor.
3. A soma com um acumulador possui dependência: cada adição precisa do resultado
   da anterior.
4. Essa espera pode criar bolhas no pipeline.
5. Usar 2, 4 e 8 acumuladores cria cadeias independentes que o processador pode
   intercalar.
6. O ganho cresceu até quatro acumuladores. Em `-O2` e `-O3`, oito acumuladores
   praticamente empataram com quatro, indicando saturação.
7. `-O2` e `-O3` habilitaram otimizações e vetorização, mas `-O3` não foi sempre
   mais rápido.
8. Todos os testes foram executados separadamente e validados com erro zero.

## Perguntas e respostas

### O que é ILP?

É o paralelismo ao nível de instrução. O processador tenta manter várias instruções
independentes em andamento dentro de um único núcleo.

### Isso é o mesmo que usar várias threads?

Não. ILP acontece dentro de um núcleo e de uma sequência de instruções. Não foram
criadas threads neste experimento.

### Onde está a dependência na soma acumulativa?

Na instrução `soma += vetor[i]`. O novo valor de `soma` depende do resultado da
iteração anterior, formando uma cadeia.

### O que é uma bolha no pipeline?

É um intervalo em que uma parte do processador fica sem trabalho útil porque a
próxima instrução ainda espera um resultado anterior.

### Como múltiplos acumuladores ajudam?

Enquanto uma soma espera seu resultado, o processador pode avançar outra soma
independente. Isso esconde parte da latência das adições.

### Quantos acumuladores foram necessários?

O ganho aumentou de um para dois e de dois para quatro. Com `-O2`, quatro levaram
4,388 ms e oito 4,355 ms, uma diferença muito pequena. Quatro já foram suficientes
para atingir praticamente o limite neste computador.

### Por que oito acumuladores não dobraram novamente o desempenho?

O processador possui uma quantidade limitada de unidades de execução e uma largura
máxima para emitir instruções. Além disso, os dados precisam vir da memória. Depois
que esses recursos ficam ocupados, adicionar mais cadeias independentes não ajuda.

### Qual foi o efeito de `-O0`?

Os laços ficaram mais lentos porque a maior parte das otimizações estava desativada.
Mesmo em `-O0`, múltiplos acumuladores ajudaram, mostrando o efeito do estilo do
código sobre o ILP.

### Qual foi o efeito de `-O2`?

Ele melhorou o uso de registradores, reduziu o custo de controle dos laços e
permitiu vetorização quando segura. A soma dependente caiu de 48,504 ms para
12,334 ms.

### Por que `-O3` não foi sempre mais rápido que `-O2`?

O compilador tenta transformações mais agressivas, mas não pode garantir que elas
serão melhores para todo laço e processador. Nas somas, os tempos de `-O2` e `-O3`
ficaram praticamente empatados.

### O compilador removeu a dependência da soma simples?

Não. Sem permitir mudanças nas regras de ponto flutuante, a cadeia de adições deve
ser preservada. O código com múltiplos acumuladores expõe explicitamente as cadeias
independentes.

### O ganho veio apenas da vetorização?

Não. Com a vetorização desativada, quatro acumuladores ainda foram cerca de 2,5
vezes mais rápidos que a soma dependente. A vetorização acrescentou aproximadamente
13% a 16% nas versões com quatro e oito acumuladores.

### Por que validar os resultados principalmente em `-O3`?

Otimizações mais agressivas podem reorganizar o código. A validação confirma que a
transformação preservou o resultado esperado. Todos os testes apresentaram erro
zero.

### Por que os laços não foram executados um logo depois do outro?

Um laço pode deixar dados na cache e favorecer o seguinte. Cada teste foi executado
em um novo processo e inicializou seu próprio vetor antes da medição.

### Por que o vetor tem aproximadamente 122 MiB?

Ele é maior que a cache compartilhada do processador. Assim, o vetor inteiro não
fica retido na cache entre passagens, reduzindo a vantagem causada pela ordem dos
testes.

### Para que servem `volatile` e `noinline`?

São proteções da medição. `volatile` torna o resultado observável e `noinline`
mantém cada laço em sua própria função, evitando que o compilador elimine ou una o
trabalho medido.

### Por que cada medição dura perto de um segundo?

Tempos muito curtos sofrem mais influência da resolução do relógio e de interrupções
do sistema. O programa dobra as repetições até acumular pelo menos 0,6 segundo; a
última duplicação normalmente deixa o total entre 0,6 e 1,2 segundo.

### Como o tempo foi medido?

Foi usado o tempo de parede com `clock_gettime(CLOCK_MONOTONIC)`. O tempo total é
dividido pelo número de repetições para obter o tempo de uma passagem pelo vetor.
