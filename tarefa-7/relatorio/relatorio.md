# Processamento de uma lista encadeada com tarefas OpenMP

## 1. Objetivo

O programa cria uma lista encadeada cujos seis nós armazenam nomes de arquivos
fictícios. Dentro de uma região paralela, o programa percorre a lista e usa
`#pragma omp task` para criar uma tarefa para cada nó. Cada tarefa imprime o nome
do arquivo e o identificador da thread que realmente a executou, obtido com
`omp_get_thread_num()`.

Foram comparadas uma implementação propositalmente incorreta e sua correção. Um
contador em cada nó permite verificar objetivamente se ele foi processado zero,
uma ou várias vezes.

## 2. Lista encadeada e tarefas

Cada nó possui o nome, um contador usado pelo experimento e um ponteiro para o
próximo nó:

```c
typedef struct No {
    char nome[64];
    int processamentos;
    struct No *proximo;
} No;
```

Uma tarefa é uma unidade de trabalho que pode ser executada posteriormente por
qualquer thread da equipe. A thread que encontra `#pragma omp task` cria a tarefa,
mas não precisa ser a thread que a executará. Por isso, o número mostrado por
`omp_get_thread_num()` identifica a executora, e a ordem das linhas não precisa
seguir a ordem da lista.

## 3. Versão incorreta: todas as threads produzem tarefas

Uma região `parallel` faz todas as threads executarem o bloco. Ela não divide
automaticamente o percurso da lista. Portanto, colocar diretamente o laço nessa
região faz cada thread visitar todos os nós:

```c
#pragma omp parallel default(none) shared(inicio)
{
    for (No *atual = inicio; atual != NULL; atual = atual->proximo) {
        #pragma omp task firstprivate(atual)
        processar_arquivo(atual, "sem single");
    }
}
```

Com quatro threads e seis nós, são criadas 24 tarefas: quatro para cada arquivo.
Nenhum nó foi ignorado no erro específico testado, porém todos foram processados
mais de uma vez. O contador é incrementado com `atomic`, de modo que a própria
verificação não introduza outra condição de corrida.

Seria ainda pior compartilhar entre as threads um único ponteiro `atual` e fazer
todas o modificarem. Isso causaria uma condição de corrida: algumas poderiam ler
um valor enquanto outra já o alterou, levando a nós ignorados, repetidos ou até
acesso inválido. Essa alternativa não foi executada porque um programa com
comportamento indefinido não é uma base segura de teste.

## 4. Correção: uma produtora, várias executoras

`single` determina que somente uma thread da equipe percorra a lista e produza as
tarefas. As outras threads não executam o bloco `single`, mas continuam livres
para retirar tarefas da fila e processá-las:

```c
#pragma omp parallel default(none) shared(inicio)
{
    #pragma omp single
    {
        for (No *atual = inicio; atual != NULL; atual = atual->proximo) {
            #pragma omp task firstprivate(atual)
            processar_arquivo(atual, "com single");
        }

        #pragma omp taskwait
    }
}
```

`firstprivate(atual)` copia para cada tarefa o valor do ponteiro no momento da
criação. Assim, o avanço do laço não muda o nó pertencente a uma tarefa já
criada. Embora o OpenMP já torne esse ponteiro local `firstprivate` nesse contexto,
declará-lo explicitamente documenta a intenção e evita que uma mudança de escopo
introduza um erro sutil.

`taskwait` faz a thread produtora aguardar suas tarefas filhas. Neste programa, a
barreira implícita no fim da região `parallel` também impediria a verificação e a
liberação da lista antes do término das tarefas; o `taskwait` foi mantido para
deixar explícito onde o conjunto produzido deve estar concluído.

## 5. Resultados e reflexão

O programa foi compilado sem avisos com:

```text
gcc -O2 -Wall -Wextra -std=c99 -fopenmp lista_tarefas.c -o lista_tarefas.exe
```

Foram feitas três execuções com quatro threads. Em todas elas foi observado:

| Versão | Tarefas criadas | Processamentos por nó | Resultado |
|---|---:|---:|---|
| sem `single` | 24 | 4 | todos os seis nós repetidos |
| com `single` | 6 | 1 | todos os seis nós exatamente uma vez |

Na versão corrigida, todos os nós foram processados; nenhum apareceu mais de uma
vez e nenhum foi ignorado. Na versão incorreta, todos apareceram quatro vezes,
uma vez para cada thread que repetiu o percurso e criou sua própria tarefa.

Entre execuções, a contagem não mudou: o defeito da primeira versão cria
exatamente uma tarefa por par *thread–nó*, enquanto a segunda cria exatamente uma
por nó. Entretanto, mudaram a ordem das mensagens e as threads que executaram
cada arquivo. Isso é esperado, pois o escalonamento das tarefas é dinâmico e não
há garantia de ordem. Uma saída diferente, isoladamente, não significa erro; os
contadores por nó é que comprovam a cobertura correta.

## 6. `single`, `nowait` e sincronização

Há uma barreira implícita ao final de `single`: em condições normais, as threads
esperam ali. A cláusula `nowait` remove essa barreira, permitindo que sigam para
o código posterior. Ela não faz várias threads executarem o bloco; o percurso
continua pertencendo a uma única produtora.

Neste caso, `nowait` não é necessário. Mesmo se fosse colocado em `single`, a
barreira implícita ao fim de `parallel` ainda aguardaria as tarefas antes de o
programa resumir os contadores e liberar a memória. Se a lista precisasse ser
verificada, alterada ou liberada ainda dentro da região paralela, seria necessário
manter uma sincronização apropriada, por exemplo `taskwait`, `taskgroup` ou uma
barreira, conforme a estrutura do programa.

`master` não é a melhor escolha para este padrão. Ele reserva o bloco para a
thread 0 e não possui barreira implícita. `single` permite que qualquer thread
disponível seja a produtora e expressa diretamente que o bloco deve ser executado
uma única vez.

## 7. Conclusão

Criar tarefas não distribui automaticamente o código que as cria. A garantia de
uma única tarefa por nó vem da combinação de três decisões: apenas uma thread
percorre a lista com `single`; cada tarefa captura seu próprio ponteiro com
`firstprivate`; e a memória permanece válida até todas as tarefas terminarem. O
runtime continua livre para escolher qualquer thread executora e qualquer ordem,
sem alterar a propriedade importante: cada nó é processado exatamente uma vez.

<div class="quebra"></div>

## 8. Código-fonte

{{CODIGO:lista_tarefas.c}}
