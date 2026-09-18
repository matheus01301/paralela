# Tutoria — Tarefa 8

## Referência

- Enunciado: [catálogo geral](../enunciados_tarefas_1_a_10.md)
- Código principal: `pi_rand_openmp.c`
- Apoio: `README.md`, `guia_apresentacao.md`, `resultados.txt` e relatório.

## Estado

Os principais conceitos foram estudados. A tarefa chegou a ser sorteada, mas a
defesa foi cancelada quando o intervalo do sorteio mudou de 6–10 para 1–10.

## Conteúdo já discutido

- Quatro versões: `rand + critical`, `rand + vetor`, `rand_r + critical` e
  `rand_r + vetor`.
- Estado global de `rand()` na glibc do experimento.
- Estado privado passado a `rand_r()` e semente diferente por thread.
- Vetor sem corrida porque cada posição possui um único escritor.
- Linha de cache de 64 bytes e oito `long long` de 8 bytes na mesma linha.
- Falso compartilhamento prejudica desempenho, não correção.
- Coerência de cache e transferências repetidas da linha.
- `volatile` preserva acessos do benchmark, mas não é atômico nem sincroniza.
- Soma serial segura após o fim da região paralela.
- Variação do pareamento dos números nas versões com `rand()`.
- Reprodutibilidade de `rand_r()` com `schedule(static)`.
- `rand_r + critical` foi mais rápido porque estado e contador ficam privados
  durante o laço, com apenas uma combinação por thread.

## Pontos a melhorar

- Explicar que `_Alignas(CACHE_LINE)` alinha somente o início do vetor; não alinha
  separadamente cada contador.
- Relacionar o vetor contíguo ao falso compartilhamento sem confundi-lo com
  condição de corrida.
- Explicar que padding por elemento ou acumulação local pode reduzir o problema.
- Memorizar os resultados somente depois de reler `resultados.txt`.

## Avaliações

Nenhuma defesa concluída e nenhuma nota atribuída.
