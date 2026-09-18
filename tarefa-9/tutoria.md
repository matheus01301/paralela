# Tutoria — Tarefa 9

## Referência

- Enunciado: [catálogo geral](../enunciados_tarefas_1_a_10.md)
- Código principal: `listas_insercoes.c`
- Apoio: `README.md`, `guia_apresentacao.md`, `resultados.txt` e relatório.

## Estado

Foi realizado estudo guiado detalhado e a primeira defesa foi concluída. O aluno
reconheceu as três versões e a necessidade do vetor de locks, mas voltou a
confundir nomes de `critical`, ciclo de vida dos locks e responsabilidades de
`omp_destroy_lock` e `free`.

## Conteúdo consolidado

- A inserção em uma lista é uma operação composta que precisa ser protegida.
- O `critical` comum mantém a correção, mas serializa as duas listas.
- `critical(lista_zero)` e `critical(lista_um)` são proteções independentes e
  permitem, sem garantir, inserções simultâneas em listas diferentes.
- O nome do `critical` é apenas um identificador fixo; ele não lê uma variável
  nem seleciona a lista. A lista é escolhida por `&listas[0]` ou `&listas[1]`.
- Quantidade dinâmica exige vetor de locks: `listas[destino]` corresponde a
  `locks[destino]`.
- Sequência: inicializar locks, criar tarefas, escolher destino, alocar nó,
  adquirir lock, inserir, liberar lock e, depois das tarefas, destruir locks.
- `calloc` reserva e zera o vetor de listas; `malloc` reserva o vetor de locks;
  `omp_init_lock` continua obrigatório.
- Padding reduz falso compartilhamento entre locks diferentes, mas não remove
  contenção verdadeira quando tarefas escolhem a mesma lista.

## Confusões corrigidas

- `critical` não é uma barreira.
- O `critical` comum é correto; apenas reduz o paralelismo.
- Nomes diferentes permitem simultaneidade, mas não a garantem.
- `critical(destino)` usa o nome literal `destino`, não o valor da variável.
- São as tarefas que acessam os locks, e não os locks que acessam tarefas.
- `omp_set_lock` ocorre antes da inserção, não no fim da tarefa.
- `omp_destroy_lock` só pode ocorrer quando nenhuma tarefa ainda usar o lock.

## Pontos a melhorar

- Explicar a generalização de forma contínua, sem inverter a relação entre tarefa
  e lock.
- Distinguir com segurança nomes fixos de `critical` e índices dinâmicos.
- Manter a ordem exata do ciclo de vida dos locks na defesa.
- Apresentar resultados com a ressalva de que oito listas não isolam apenas a
  diferença entre locks e `critical`.

## Avaliações

### Defesa de 15/09/2026

**Nota: 6,5/10.**

Acertos demonstrados:

- identificou as três versões do programa;
- explicou que o `critical` comum protege corretamente, mas serializa as duas
  listas;
- reconheceu que dois `critical` nomeados removem a serialização desnecessária
  entre listas diferentes;
- explicou que nomes de `critical` não podem ser calculados por índice;
- identificou a alocação de um vetor de listas e outro com um lock por lista;
- apresentou a ordem geral de inicialização, aquisição, liberação e destruição.

Erros ou imprecisões observados:

- repetiu que cada lista é associada a uma thread, mas as tarefas podem ser
  executadas por qualquer thread e não há associação permanente;
- tratou o nome do `critical` como se nomeasse ou referenciasse a lista; ele é
  apenas um identificador fixo da proteção;
- não deixou claro que nomes diferentes apenas permitem, sem garantir, execução
  simultânea;
- disse que a generalização usa `critical`; ela substitui as regiões nomeadas por
  locks explícitos indexáveis;
- não explicou a operação composta de inserção nem como a corrida perde nós;
- descreveu `taskgroup` como criador de tarefas; `task` cria e `taskgroup` agrupa
  e espera as tarefas do bloco;
- afirmou que `omp_destroy_lock` não pode estar dentro de um `for`, mas o código
  usa um laço para destruir cada lock depois que todas as tarefas terminaram;
- inverteu a liberação de recursos: `omp_destroy_lock` encerra os recursos do
  runtime, enquanto `free(locks)` libera a memória do vetor;
- omitiu validação, padding contra falso compartilhamento e resultados medidos.

A defesa foi concluída com necessidade de revisão antes de nova tentativa.
