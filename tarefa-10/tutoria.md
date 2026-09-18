# Tutoria — Tarefa 10

## Referência

- Enunciado: [catálogo geral](../enunciados_tarefas_1_a_10.md)
- Código principal: `pi_sincronizacao.c`
- Apoio: `README.md`, `guia_apresentacao.md`, `resultados.txt` e relatório.

## Estado

Estudo guiado concluído. O aluno declarou conseguir explicar toda a tarefa.
Defesa formal ainda pendente.

## Conteúdo consolidado

- Cinco versões: compartilhado com `critical`, compartilhado com `atomic`,
  privado com `critical`, privado com `atomic` e `reduction`.
- Nas versões compartilhadas ocorre uma atualização por hit, aproximadamente
  15,7 milhões no experimento.
- `atomic` compartilhado reduz o custo por atualização, mas não a quantidade nem
  a contenção no contador.
- Nas versões privadas, cada thread acumula localmente e combina apenas uma vez;
  com oito threads, são oito atualizações globais.
- O `critical` ou `atomic` da versão privada fica depois do `omp for`, ainda
  dentro da região `parallel`.
- `reduction(+:hits)` cria cópias privadas e automatiza a combinação final.
- Zero atualizações explicitamente sincronizadas na versão `reduction` não
  significa ausência de combinação ou sincronização interna.
- Para esta soma, `reduction` é a opção mais clara e produtiva.

## Confusões corrigidas

- O primeiro `critical` ocorre por hit, não por todos os pontos.
- Na Tarefa 8, `rand_r_critical` faz uma soma protegida por thread.
- Na versão privada, o `critical` não está dentro do laço `for`.
- A `reduction` automatiza a combinação; não a elimina.

## Pontos a melhorar na defesa

- Explicar o roteiro completo de escolha entre `reduction`, privatização,
  `atomic`, `critical`, `critical` nomeado e locks explícitos.
- Separar custo de cada sincronização da frequência de sincronização.
- Citar resultados reais sem afirmar um vencedor universal entre as três versões
  com mediana de 0,019 s.

## Avaliações

Nenhuma defesa concluída e nenhuma nota atribuída.
