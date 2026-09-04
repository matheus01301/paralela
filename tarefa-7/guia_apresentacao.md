# Guia para apresentação — Tarefa 7

Este arquivo é um apoio para a explicação presencial e não faz parte do PDF.

## Fala curta

> `parallel` faz todas as threads executarem o bloco. Por isso, se eu colocar o
> percurso da lista diretamente nele, cada thread percorre todos os nós e cria
> tarefas repetidas. A correção é usar `single`: uma única thread percorre a lista
> e cria uma tarefa por nó, enquanto qualquer thread da equipe pode executar as
> tarefas. `firstprivate(atual)` registra em cada tarefa o ponteiro do seu nó, e
> `taskwait` deixa explícito que todas precisam terminar antes de prosseguir.

## Ordem para explicar o código

1. `No` guarda nome, contador de verificação e endereço do próximo nó.
2. `criar_lista` aloca e liga seis nós; `liberar_lista` os desaloca no final.
3. `processar_arquivo` obtém a thread executora e incrementa o contador do nó.
4. `executar_sem_single` mostra o erro: todas as threads percorrem toda a lista.
5. `executar_com_single` possui uma produtora e várias possíveis executoras.
6. `mostrar_resumo` comprova quantas vezes cada nó foi processado.

## Perguntas e respostas

### O que `parallel` faz?

Cria uma equipe e faz todas as threads executarem o bloco. Sozinho, não reparte o
percurso de uma lista.

### O que `task` faz?

Cria uma unidade de trabalho que entra na fila do runtime. Ela pode ser executada
imediatamente ou depois, pela thread criadora ou por outra thread da equipe.

### A thread que cria também executa a tarefa?

Pode executar, mas não há essa garantia. O identificador impresso é obtido dentro
da tarefa e corresponde à thread executora.

### Por que não usar `omp for` no percurso?

`omp for` distribui iterações de um laço canônico, normalmente indexado. Um
percurso `atual = atual->proximo` de lista encadeada não é um laço canônico do
OpenMP. O padrão natural é uma thread percorrer e gerar tarefas.

### Por que a primeira versão repete os nós?

Porque cada thread executa o mesmo laço inteiro e cria uma tarefa para cada nó.
Com quatro threads, há quatro tarefas por nó.

### O que `single` faz?

Escolhe uma thread qualquer da equipe para executar o bloco uma vez. As demais
podem trabalhar nas tarefas criadas.

### `single` significa thread 0?

Não. Qualquer thread pode ser escolhida. `master` é que reserva o bloco para a
thread 0.

### Qual a diferença entre `single` e `master`?

Além de `master` exigir a thread 0, ele não possui barreira implícita. `single`
tem uma barreira implícita no final, exceto quando recebe `nowait`.

### O que `nowait` faria?

Removeria a barreira implícita de `single`. Não mudaria a quantidade de threads
que percorrem a lista. Neste programa não é necessário; a barreira no fim de
`parallel` ainda impede o uso ou a liberação prematura da lista fora da região.

### Para que serve `firstprivate(atual)`?

Copia o valor atual do ponteiro para a tarefa no instante em que ela é criada.
Cada tarefa fica ligada ao nó correto mesmo que o laço avance antes de ela rodar.

### Para que serve `taskwait`?

A thread que o encontra espera suas tarefas filhas diretas. Aqui ele explicita
que todas as tarefas geradas no `single` terminaram. A barreira final de
`parallel` já seria suficiente antes de verificar e liberar a lista fora dela.

### `taskwait` faz as tarefas executarem em ordem?

Não. Apenas espera o término; a ordem e a thread executora continuam livres.

### Por que o contador usa `atomic`?

Na versão errada, várias tarefas incrementam o mesmo nó. Sem proteção, a própria
medição teria uma condição de corrida e poderia perder incrementos. `atomic`
torna o diagnóstico confiável; ele não corrige a duplicação de tarefas.

### Por que há um `critical` no `printf`?

Somente para evitar que linhas de saída de tarefas simultâneas se misturem. Ele
não determina a ordem e não é a garantia de processamento único.

### O comportamento muda entre execuções?

A ordem e os identificadores das threads podem mudar. Com quatro threads, a
primeira versão continua criando quatro tarefas por nó e a correta continua
criando uma; a cobertura é determinística, embora o escalonamento não seja.

### Um nó poderia ser ignorado?

Não nessas duas versões enquanto a lista não for modificada. Porém, usar um
ponteiro de percurso compartilhado e alterado por várias threads criaria uma
corrida e poderia ignorar ou repetir nós.

## O que realmente precisa ser memorizado

1. `parallel` replica o bloco; não divide automaticamente uma lista.
2. `single` cria uma única produtora de tarefas.
3. `task` pode ser executada por qualquer thread.
4. `firstprivate(atual)` associa cada tarefa ao nó correto.
5. Esperar as tarefas antes de liberar a lista mantém os ponteiros válidos.
6. Ordem variável não é duplicação; os contadores verificam cada nó.
