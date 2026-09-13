# Guia de apresentação — Tarefa 9

## Resumo em 30 segundos

O programa cria `N` tarefas; cada uma escolhe uma lista e insere um nó no início.
A leitura e a escrita do ponteiro do início precisam ser atômicas como operação
composta. Para duas listas fixas, usei dois `critical` nomeados, permitindo uma
inserção em cada lista ao mesmo tempo. Para uma quantidade dinâmica, usei um vetor
de `omp_lock_t`, porque o nome de um `critical` precisa existir no código-fonte e
não pode ser calculado com um índice.

## Pontos para demonstrar no código

1. `single` faz apenas uma thread produzir as tarefas; qualquer thread da equipe
   pode executá-las.
2. `firstprivate(i)` dá a cada tarefa seu próprio número.
3. A escolha pseudoaleatória não compartilha estado de `rand()`.
4. `critical(lista_zero)` e `critical(lista_um)` são locks implícitos distintos.
5. Na versão geral, `locks[destino]` associa dinamicamente um lock a cada lista.
6. `taskgroup` termina antes da validação, do `omp_destroy_lock` e de `free`.

## Perguntas prováveis

**Por que não basta `atomic` no ponteiro?**

A inserção é uma operação composta: ler o início antigo, gravá-lo em `proximo`,
publicar o novo início e atualizar o tamanho. Um único `atomic` não protege todo
esse invariante como a mesma transação.

**Por que o `critical` comum é correto, mas menos escalável?**

Ele serializa todas as inserções. É seguro, porém uma tarefa na lista 0 bloqueia
outra na lista 1 mesmo sem compartilharem dados.

**O nome do `critical` poderia ser `critical(lista[i])`?**

Não. O nome é um identificador fixo da diretiva, conhecido na compilação. Para um
número lido do usuário seria preciso codificar casos manualmente; o vetor de locks
é a solução dinâmica.

**Por que chamar `omp_destroy_lock` se o processo vai terminar?**

Para encerrar corretamente os recursos do runtime e permitir que a função seja
reutilizada sem vazamentos. Depois de `destroy`, ainda é necessário `free` no
vetor: as duas chamadas liberam recursos diferentes.

**O que é falso compartilhamento aqui?**

Dois locks diferentes podem ocupar a mesma linha de cache de 64 bytes. Cada
aquisição escreve nessa linha e a faz migrar entre caches, criando contenção de
hardware sem contenção lógica. A estrutura adiciona 64 bytes de separação entre
locks consecutivos; os cabeçalhos das listas recebem tratamento semelhante.

**O `critical` nomeado sempre é mais rápido?**

Não. Ele permite mais paralelismo, mas o trecho crítico é muito curto e o tempo
também inclui tarefas e alocação. Nas cinco medições, as medianas foram 0,563 s
para o nomeado e 0,577 s para o comum, ganho de apenas 2,4%; algumas rodadas
favoreceram cada versão.

**Como a integridade foi verificada?**

Após todas as tarefas, o programa compara o contador protegido de cada lista com
o número de nós realmente percorridos e com `N` menos falhas de alocação.

## Comandos

```powershell
cd tarefa-9
.\executar_testes.ps1 -Insercoes 200000 -Threads 8 -Listas 8 -Execucoes 5
cd relatorio
python build_pdf.py
```
