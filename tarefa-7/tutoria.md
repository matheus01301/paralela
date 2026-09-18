# Tutoria — Tarefa 7

## Referência

- Enunciado: [catálogo geral](../enunciados_tarefas_1_a_10.md)
- Código principal: `lista_tarefas.c`
- Apoio: `README.md`, `guia_apresentacao.md`, `resultados.txt` e relatório.

## Estado

Estudo conceitual concluído anteriormente. Defesa formal ainda pendente.

## Conteúdo já discutido

- Lista encadeada e criação de tarefas com `task`.
- Uso de `single` para evitar criação duplicada de tarefas.
- `firstprivate(atual)` para capturar o nó correto em cada tarefa.
- Diferença entre thread produtora e thread executora.
- `taskwait` e término das tarefas.
- Contador de processamentos com `atomic capture`.
- Saída protegida por `critical(saida)`.
- Conceitos relacionados: `nowait`, `master`, `malloc` e `free`.

## Pontos a observar na defesa

- Não afirmar que a thread que cria uma tarefa necessariamente a executa.
- Explicar por que a ausência de `single` duplica a travessia e as tarefas.
- Explicar por que o ponteiro do nó precisa ser capturado corretamente.
- Basear resultados e nomes nos arquivos reais.

## Avaliações

Nenhuma defesa concluída e nenhuma nota atribuída.
