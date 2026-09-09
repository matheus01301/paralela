# Guia de apresentação — Tarefa 8

## Resumo em 30 segundos

“Estimei π com pontos aleatórios e comparei quatro combinações. `rand()` usa
estado compartilhado e, na glibc, as chamadas concorrentes são serializadas.
`rand_r()` recebe uma semente por thread e elimina esse gargalo. O contador
privado com um `critical` por thread foi o mais rápido. O vetor não tem corrida,
mas seus contadores contíguos compartilham linhas de cache, causando falso
compartilhamento.”

## O que apontar no código

1. `acertos_locais` nasce dentro da região paralela e é privado.
2. O `critical` está fora do `omp for`: ocorre somente uma vez por thread.
3. No vetor, o índice é exclusivo, mas oito `long long` cabem em 64 bytes.
4. `volatile` conserva cada incremento na memória para observar o fenômeno; não
   torna uma operação atômica.
5. Em `rand_r(&estado)`, `estado` é uma variável automática privada e cada thread
   começa com semente diferente.
6. A soma do vetor acontece após a região paralela e é serial.

## Números principais

Com 10 milhões de pontos e 16 threads, as medianas foram:

| Versão | Tempo |
|---|---:|
| `rand + critical` | 0,839048 s |
| `rand + vetor` | 0,947555 s |
| `rand_r + critical` | 0,013797 s |
| `rand_r + vetor` | 0,038935 s |

`rand_r + vetor` foi 2,82 vezes mais lento que `rand_r + critical`, evidenciando
o custo do falso compartilhamento quando a serialização do gerador desaparece.

## Perguntas prováveis

**Por que o vetor tem falso compartilhamento se cada thread usa outro índice?**

Porque a coerência é feita por linha de cache. Índices diferentes ainda podem
estar nos mesmos 64 bytes; uma escrita invalida a linha inteira nos outros
núcleos.

**Há condição de corrida no vetor?**

Não. Cada posição tem um único escritor e a soma só começa depois da barreira
implícita ao fim da região paralela. Falso compartilhamento prejudica desempenho,
não correção.

**Por que `critical` venceu?**

O contador é privado durante milhões de iterações e cada thread entra no
`critical` apenas uma vez. São 16 atualizações protegidas, contra milhões de
escritas coerentes no vetor.

**Por que `rand()` é lento?**

Na glibc usada, todas as threads disputam o estado global protegido do gerador em
cada chamada. São duas chamadas por ponto. Essa escolha é específica da
biblioteca; não se deve presumir o mesmo custo em toda plataforma.

**Por que as versões `rand()` não repetem exatamente o resultado com a mesma
semente?**

A sequência global é a mesma, mas a intercalação das chamadas muda. Como duas
chamadas formam cada ponto, o pareamento dos números e sua atribuição às
iterações variam conforme o escalonamento.

**Por que `rand_r()` repete o resultado?**

Cada thread tem sua própria sequência e `schedule(static)` atribui a ela a mesma
quantidade de trabalho em todas as execuções.

**Como eliminar o falso compartilhamento?**

Separando contadores por pelo menos uma linha de cache (*padding/alinhamento*) ou
contando em variável privada e gravando no vetor uma única vez ao final.

**`volatile` resolve sincronização?**

Não. Ele apenas impede a eliminação/promoção dos acessos pelo compilador. Não
torna `++` atômico e não substitui `critical`, `atomic` ou `reduction` quando há
múltiplos escritores.
