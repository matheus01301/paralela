# Enunciados das tarefas 1 a 10

## Tarefa 1: Aproximação matemática de PI

Implemente um programa em C que calcule uma aproximação de π usando uma série matemática, variando o número de iterações e medindo o tempo de execução. Compare os valores obtidos com o valor real de π e analise como a acurácia melhora com mais processamento. Reflita sobre como esse comportamento se repete em aplicações reais que demandam resultados cada vez mais precisos, como simulações físicas e inteligência artificial.

## Tarefa 2: Localidade temporal e espacial

Implemente duas versões da multiplicação de matriz por vetor (MxV) em C: uma com acesso à matriz por linhas (laço interno variando a coluna) e outra por colunas (laço interno variando a linha). Meça o tempo de execução de cada versão usando uma função apropriada e execute testes com matrizes de diferentes tamanhos. Identifique a partir de qual tamanho os tempos passam a divergir significativamente e explique por que isso ocorre, relacionando suas observações ao uso da memória cache e ao padrão de acesso à memória.

## Tarefa 3: Pipeline e vetorização

Implemente três laços em C para investigar os efeitos do paralelismo ao nível de instrução (ILP): 1) inicialize um vetor com um cálculo simples; 2) some seus elementos de forma acumulativa, criando dependência entre as iterações; e 3) quebre essa dependência utilizando múltiplas variáveis. Compare o tempo de execução das versões compiladas com diferentes níveis de otimização (O0, O2, O3) e analise como o estilo do código e as dependências influenciam o desempenho.

## Tarefa 4: Aplicações limitadas por memória ou CPU

Implemente dois programas paralelos em C com OpenMP: um limitado por memória, com somas simples em vetores, e outro limitado por CPU, com cálculos matemáticos intensivos. Paralelize com `#pragma omp parallel for` e meça o tempo de execução variando o número de threads. Analise quando o desempenho melhora, estabiliza ou piora, e reflita sobre como o multithreading de hardware pode ajudar em programas memory-bound, mas atrapalhar em programas compute-bound pela competição por recursos.

## Tarefa 5: Comparação entre programação sequencial e paralela

Implemente um programa em C que conte quantos números primos existem entre 2 e um valor máximo `n`. Depois, paralelize o laço principal usando a diretiva `#pragma omp parallel for` sem alterar a lógica original. Compare o tempo de execução e os resultados das versões sequencial e paralela. Observe possíveis diferenças no resultado e no desempenho, e reflita sobre os desafios iniciais da programação paralela, como correção e distribuição de carga.

## Tarefa 6: Escopo de variáveis e regiões críticas

Implemente em C a estimativa estocástica de π. Paralelize com `#pragma omp parallel for` e explique o resultado incorreto. Corrija a condição de corrida utilizando o `#pragma omp critical` e reestruturando com `#pragma omp parallel` seguido de `#pragma omp for` e aplicando as cláusulas `private`, `firstprivate`, `lastprivate` e `shared`. Teste diferentes combinações e explique como cada cláusula afeta o comportamento do programa. Comente também como a cláusula `default(none)` pode ajudar a tornar o escopo mais claro em programas complexos.

## Tarefa 7: Processamento Paralelo de Lista Encadeada com OpenMP Task

Implemente um programa em C que cria uma lista encadeada com nós, cada um contendo o nome de um arquivo fictício. Dentro de uma região paralela, percorra a lista e crie uma tarefa com `#pragma omp task` para processar cada nó. Cada tarefa deve imprimir o nome do arquivo e o identificador da thread que a executou com o `omp_get_thread_num`. Após executar o programa, reflita: todos os nós foram processados? Algum foi processado mais de uma vez ou ignorado? O comportamento muda entre execuções? Como garantir que cada nó seja processado uma única vez e por apenas uma tarefa?

## Tarefa 8: Coerência de Cache e Falso Compartilhamento

Implemente estimativa estocástica de π usando `rand()` para gerar os pontos. Cada thread deve usar uma variável privada para contar os acertos e acumular o total em uma variável global com `#pragma omp critical`. Depois, implemente uma segunda versão em que cada thread escreve seus acertos em uma posição distinta de um vetor compartilhado. A acumulação deve ser feita em um laço serial após a região paralela. Compare o tempo de execução das duas versões. Em seguida, substitua `rand()` por `rand_r()` em ambas e compare novamente. Explique o comportamento dos quatro programas com base na coerência de cache e nos efeitos do falso compartilhamento.

## Tarefa 9: Regiões críticas nomeadas e Locks explícitos

Escreva um programa que cria tarefas para realizar N inserções em duas listas encadeadas, cada uma associada a uma thread. Cada tarefa deve escolher aleatoriamente em qual lista inserir um número. Garanta a integridade das listas evitando condição de corrida e, sempre que possível, use regiões críticas nomeadas para que a inserção em uma lista não bloqueie a outra. Em seguida, generalize o programa para um número de listas definido pelo usuário. Explique por que, nesse caso, regiões críticas nomeadas não são suficientes e por que o uso de locks explícitos se torna necessário.

## Tarefa 10: Mecanismos de Sincronização com OpenMP

Implemente novamente o estimador da tarefa 8 usando um contador compartilhado e o `rand_r`, protegendo a soma de hits com `#pragma omp critical` e com `#pragma omp atomic`. Compare essas duas implementações com suas versões que usam contadores privados. Agora, compare essas com uma quinta versão que utiliza apenas a cláusula `reduction` ao invés das diretivas de sincronização. Reflita sobre a aplicabilidade desses mecanismos em termos de desempenho e produtividade e proponha um roteiro para quando utilizar qual mecanismo de sincronização, incluindo `critical` nomeadas e locks explícitos.

## Requisito geral dos relatórios

O relatório deve ser entregue em formato PDF e conter o código utilizado para a resolução da questão no próprio arquivo, com a sintaxe colorida (realce de código).
