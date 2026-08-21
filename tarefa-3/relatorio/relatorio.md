# Dependências, ILP e otimização de laços em C

## 1. Objetivo

O objetivo desta tarefa foi investigar o paralelismo ao nível de instrução
(*Instruction-Level Parallelism*, ILP). Foram implementados um laço de
inicialização, uma soma com dependência entre iterações e versões da soma com 2,
4 e 8 acumuladores independentes. Os tempos foram comparados com os níveis de
otimização `-O0`, `-O2` e `-O3`.

## 2. Os três tipos de laço

No laço de **inicialização**, cada iteração calcula e escreve uma posição diferente
do vetor. Uma iteração não precisa do resultado da anterior, permitindo que o
processador execute várias instruções ao mesmo tempo.

Na **soma dependente**, existe apenas um acumulador:

<div class="formula">soma ← soma + vetor[i]</div>

Cada adição precisa esperar o valor de `soma` produzido pela adição anterior. Essa
cadeia cria uma dependência verdadeira. Enquanto o resultado não fica pronto,
podem surgir espaços sem trabalho útil, conhecidos como **bolhas no pipeline**.

O terceiro estilo quebra a cadeia usando acumuladores independentes. Com quatro
variáveis, por exemplo, `soma0`, `soma1`, `soma2` e `soma3` podem avançar sem esperar
umas pelas outras. Ao final, elas são combinadas para formar o resultado.

Foram testados 2, 4 e 8 acumuladores para identificar até que ponto criar cadeias
independentes continua trazendo ganho.

## 3. Metodologia

- Vetor com 16 milhões de elementos do tipo `double`, ocupando aproximadamente
  122 MiB.
- Tempo de parede medido com `clock_gettime(CLOCK_MONOTONIC)`.
- Quantidade de repetições dobrada automaticamente até cada medição durar pelo
  menos 0,6 segundo.
- Três execuções por combinação; a tabela apresenta a mediana.
- Cada teste executado em um novo processo. Assim, a soma dependente e as somas
  múltiplas não são medidas uma imediatamente depois da outra.
- Compilação com GCC 16.1.0 e as mesmas opções, mudando apenas `-O0`, `-O2` ou
  `-O3`.
- Resultado observado por uma variável `volatile` e funções marcadas como
  `noinline`, evitando que o compilador elimine ou junte os testes.

O vetor é inicializado por cada processo antes da soma. Como ele é maior que a
cache compartilhada do processador, não permanece inteiro na cache. Isso reduz a
possibilidade de um teste ser favorecido pelo acesso realizado no teste anterior.

## 4. Resultados

Tempos medianos de uma passagem completa pelo vetor, em milissegundos:

| Laço | `-O0` | `-O2` | `-O3` | Ganho O0 → O3 |
|---|---:|---:|---:|---:|
| Inicialização | 22,324 | 7,242 | 6,528 | 3,42× |
| Soma dependente | 48,504 | 12,334 | 12,405 | 3,91× |
| 2 acumuladores | 24,843 | 6,742 | 6,633 | 3,75× |
| 4 acumuladores | 13,017 | 4,388 | 4,390 | 2,97× |
| 8 acumuladores | 10,070 | 4,355 | 4,313 | 2,33× |

Todas as versões retornaram `64000012000000,0`, com erro zero em todos os níveis
de otimização, incluindo `-O3`.

### 4.1 Ganho ao quebrar a dependência

Tomando a soma dependente como referência:

| Acumuladores | `-O0` | `-O2` | `-O3` |
|---:|---:|---:|---:|
| 1 | 1,00× | 1,00× | 1,00× |
| 2 | 1,95× | 1,83× | 1,87× |
| 4 | 3,73× | 2,81× | 2,83× |
| 8 | 4,82× | 2,83× | 2,88× |

Duas variáveis quase dividiram o tempo por dois. Quatro produziram novo ganho.
Com `-O2` e `-O3`, passar de quatro para oito praticamente não alterou o tempo.
Nesse ponto, mais acumuladores não escondem latência adicional: a largura do
pipeline, as unidades de execução e a transferência de memória já estão próximas
do limite útil para esse laço.

## 5. Efeito dos níveis de otimização

`-O0` mantém o código próximo da forma escrita e desativa a maior parte das
transformações. Mesmo assim, separar os acumuladores melhorou o tempo, mostrando
que o estilo do código já expõe instruções independentes ao processador.

`-O2` reduziu fortemente todos os tempos por meio de melhor uso de registradores,
eliminação de trabalho de controle e vetorização automática quando permitida.
Neste GCC, as opções de vetorização estão habilitadas tanto em `-O2` quanto em
`-O3`.

`-O3` não garantiu um programa mais rápido. Ele melhorou um pouco a inicialização,
mas empatou com `-O2` nas somas; a soma dependente chegou a ficar ligeiramente mais
lenta dentro da variação experimental. Otimização é uma tentativa do compilador,
e seu resultado depende do laço e do processador.

## 6. Validação da vetorização

Foi feito um teste de controle com `-O2 -fno-tree-vectorize`, desativando a
vetorização automática:

| Laço | `-O2` normal | `-O2` sem vetorização |
|---|---:|---:|
| Soma dependente | 12,334 ms | 12,677 ms |
| 2 acumuladores | 6,742 ms | 6,686 ms |
| 4 acumuladores | 4,388 ms | 5,039 ms |
| 8 acumuladores | 4,355 ms | 5,202 ms |

A soma dependente e a versão com dois acumuladores quase não mudaram. Com quatro
e oito acumuladores, a vetorização trouxe cerca de 13% a 16% de ganho adicional.
Portanto, a melhoria principal veio da quebra da dependência e do ILP; nas versões
maiores, o compilador também aproveitou SIMD.

## 7. Conclusão

A dependência no único acumulador limita o paralelismo porque cada adição espera a
anterior. Usar acumuladores independentes permite intercalar instruções e reduz as
bolhas no pipeline. O ganho cresceu até quatro acumuladores e depois saturou nas
compilações otimizadas. `-O2` e `-O3` foram muito superiores a `-O0`, mas `-O3`
não foi automaticamente melhor que `-O2`. Os resultados mostram que desempenho
depende tanto do código escrito quanto das transformações escolhidas pelo
compilador.

<div class="quebra"></div>

## 8. Código-fonte

{{CODIGO:ilp.c}}
