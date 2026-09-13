# Inserções concorrentes em listas encadeadas com OpenMP

## 1. Objetivo e organização

O programa cria `N` tarefas OpenMP. Cada tarefa produz um nó contendo seu número,
escolhe pseudoaleatoriamente uma lista e o insere no início dela. Como várias
tarefas podem ler e modificar simultaneamente o ponteiro `inicio`, a atualização
precisa ser mutuamente exclusiva: sem proteção, duas tarefas poderiam ler o mesmo
início, gravar sucessores diferentes e uma delas sobrescrever a atualização da
outra, perdendo um nó.

Foram implementadas e medidas três versões:

| Versão | Listas | Proteção |
|---|---:|---|
| comparação | 2 | um `critical` sem nome para ambas |
| solução específica | 2 | `critical(lista_zero)` e `critical(lista_um)` |
| generalização | definida pelo usuário | um `omp_lock_t` por lista |

As listas são recursos compartilhados. Uma tarefa pode ser executada por qualquer
thread da equipe; o OpenMP não fixa permanentemente uma lista ou uma tarefa a uma
thread. O importante é que tarefas destinadas a listas distintas possam progredir
simultaneamente quando a forma de sincronização permitir.

## 2. Criação das tarefas e escolha aleatória

Somente uma thread cria as `N` tarefas, evitando que cada thread repita todo o
laço. `firstprivate(i)` dá a cada tarefa uma cópia do seu identificador, enquanto
as listas e seus mecanismos de proteção permanecem compartilhados. O `taskgroup`
garante que todas as inserções acabaram antes da validação ou liberação da memória.

```c
#pragma omp parallel default(none) shared(/* dados compartilhados */)
{
    #pragma omp single
    {
        #pragma omp taskgroup
        {
            for (long long i = 0; i < insercoes; i++) {
                #pragma omp task firstprivate(i)
                {
                    int destino = escolher_lista(i, semente, quantidade);
                    /* alocação, aquisição da proteção e inserção */
                }
            }
        }
    }
}
```

A função `escolher_lista` mistura o identificador único da tarefa com uma semente
e calcula o resto pelo número de listas. Isso fornece uma distribuição
pseudoaleatória reproduzível sem compartilhar o estado de `rand()`. Portanto, a
própria escolha não introduz uma condição de corrida e as três versões recebem a
mesma sequência de destinos quando usam a mesma semente.

## 3. Duas listas com regiões críticas nomeadas

A inserção altera dois campos relacionados, `inicio` e `tamanho`, sob a mesma
proteção:

```c
static void inserir_sem_protecao(Lista *lista, No *novo)
{
    novo->proximo = lista->inicio;
    lista->inicio = novo;
    lista->tamanho++;
}
```

Um `critical` sem nome usa o mesmo lock implícito que todos os outros `critical`
sem nome do programa. Consequentemente, uma inserção na lista 0 bloqueia outra na
lista 1 mesmo sem haver conflito de dados:

```c
#pragma omp critical
inserir_sem_protecao(&listas[destino], novo);
```

Com duas listas conhecidas ao compilar, dois nomes resolvem a serialização
desnecessária:

```c
if (destino == 0) {
    #pragma omp critical(lista_zero)
    inserir_sem_protecao(&listas[0], novo);
} else {
    #pragma omp critical(lista_um)
    inserir_sem_protecao(&listas[1], novo);
}
```

Regiões com o mesmo nome são mutuamente exclusivas; nomes diferentes representam
domínios de sincronização diferentes. Assim, duas tarefas destinadas à mesma
lista continuam serializadas, preservando sua integridade, enquanto tarefas em
listas distintas podem executar as pequenas atualizações ao mesmo tempo.

## 4. Generalização e necessidade de locks explícitos

O nome de uma região `critical` é um identificador escrito no código-fonte. Ele
não pode ser um índice, uma string construída durante a execução ou um elemento
de vetor. Seria possível escrever manualmente um grande `switch` com muitos nomes,
mas isso imporia um limite decidido na compilação e não atenderia a uma quantidade
arbitrária informada pelo usuário.

A generalização aloca um vetor de listas e outro com exatamente um lock por lista.
O índice pseudoaleatório seleciona tanto a lista como sua proteção:

```c
int destino = escolher_lista(i, semente, quantidade);
omp_set_lock(&locks[destino].valor);
inserir_sem_protecao(&listas[destino], novo);
omp_unset_lock(&locks[destino].valor);
```

O ciclo de vida de cada lock é explícito. `omp_init_lock` deve ser chamado antes
de qualquer tarefa usá-lo; `omp_set_lock` adquire; `omp_unset_lock` libera; e
`omp_destroy_lock` encerra o objeto depois do `taskgroup`. `destroy` não substitui
`free`: ele libera eventuais recursos do runtime associados ao lock, enquanto
`free` libera os vetores alocados. Embora o sistema operacional recupere memória
ao terminar o processo, omitir essas operações é uma prática incorreta, prejudica
ferramentas de diagnóstico e falha se a rotina for reutilizada por um programa
que continua executando.

## 5. Falso compartilhamento dos locks

Locks logicamente independentes também podem interferir pelo hardware. Se vários
`omp_lock_t` pequenos e contíguos ocuparem a mesma linha de cache, uma thread que
modifica o lock da lista 0 invalida a linha inteira nos caches dos outros núcleos.
Outra thread que modifica o lock da lista 1 precisa então transferir novamente a
mesma linha, apesar de as duas listas serem independentes. Esse tráfego de
coerência é chamado **falso compartilhamento**.

Na máquina do experimento, a linha de cache possui 64 bytes. Por isso o programa
envolve cada lock em uma estrutura com 64 bytes adicionais:

```c
#define CACHE_LINE 64
typedef struct {
    omp_lock_t valor;
    unsigned char separacao[CACHE_LINE];
} LockSeparado;
```

O início de dois locks consecutivos fica separado por mais de 64 bytes, impedindo
que ambos caiam na mesma linha de 64 bytes. Os cabeçalhos das listas recebem
separação equivalente, pois permitir inserções simultâneas e depois colocar os
ponteiros `inicio` na mesma linha de cache recriaria o mesmo gargalo. O valor 64 é
uma hipótese documentada da plataforma medida; a linguagem C e o OpenMP não
oferecem um tamanho universal de linha de cache.

## 6. Verificação de integridade

Depois que o `taskgroup` termina, cada lista é percorrida sem concorrência. O
programa compara três valores: a soma dos campos `tamanho`, a quantidade real de
nós alcançados pelos ponteiros e `N` menos eventuais falhas de alocação. O estado
só é marcado `OK` se os três coincidirem. Isso detecta o sintoma típico da corrida
na inserção, em que um encadeamento é sobrescrito e nós deixam de ser alcançáveis.

Uma falha de `malloc` é contada com `atomic`, pois o contador de falhas também é
compartilhado. O nó só entra na região crítica depois de ser alocado; dessa forma,
o lock protege apenas a alteração estrutural curta da lista.

## 7. Resultados e análise de desempenho

O código foi compilado sem avisos com:

```text
gcc -O2 -Wall -Wextra -std=c11 -fopenmp listas_insercoes.c -o listas_insercoes.exe
```

Foram realizadas cinco execuções com 200.000 inserções, oito threads e oito
listas na versão generalizada. Todas as 15 combinações de versão e repetição
armazenaram 200.000 nós, não tiveram falha de alocação e terminaram com
integridade `OK`.

| Versão | Tempos das 5 execuções (s) | Mediana (s) |
|---|---|---:|
| 2 listas, `critical` comum | 0,576; 0,581; 0,588; 0,560; 0,577 | 0,577 |
| 2 listas, `critical` nomeado | 0,560; 0,571; 0,546; 0,563; 0,583 | 0,563 |
| 8 listas, locks explícitos | 0,408; 0,426; 0,439; 0,438; 0,362 | 0,426 |

Na mediana, nomear as duas regiões reduziu cerca de 2,4% do tempo em
relação ao `critical` comum, diferença pequena demais para afirmar um ganho
estável. A possibilidade de duas inserções simultâneas existe, mas o trecho
protegido contém apenas duas atualizações de ponteiro/contador; criação e
escalonamento das tarefas, `malloc`, `free` e tráfego de cache continuam presentes.
Além disso, dois locks permitem mais concorrência, não garantem menor tempo: em
algumas repetições o nomeado foi mais lento por variação de escalonamento e custo
de coerência. Portanto, a vantagem principal é remover uma dependência falsa; o
efeito temporal depende da carga e deve ser medido.

Com oito listas, os locks explícitos obtiveram mediana 26,2% menor que o
`critical` comum de duas listas. Essa comparação também muda a quantidade de
listas e, consequentemente, reduz a probabilidade de contenção; ela demonstra a
escalabilidade da generalização, mas não isola somente o custo da API de locks.

## 8. Conclusão

As inserções são corretas porque toda alteração em um cabeçalho ocorre enquanto a
proteção exclusiva daquela lista está adquirida e porque a memória permanece
válida até o fim das tarefas. Para duas listas fixas, regiões críticas nomeadas
expressam com clareza duas exclusões mútuas independentes. Para uma quantidade
conhecida apenas em execução, nomes de diretivas não podem ser gerados
dinamicamente; um vetor de `omp_lock_t` fornece exatamente o mapeamento necessário
entre índice e lock. Separar fisicamente locks e cabeçalhos evita que a coerência
de cache volte a acoplar estruturas que o algoritmo tornou independentes.

<div class="quebra"></div>

## 9. Código-fonte completo

{{CODIGO:listas_insercoes.c}}
