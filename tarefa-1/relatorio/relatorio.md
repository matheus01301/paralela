# Aproximação de π e tempo de execução

## 1. Objetivo

O objetivo desta tarefa foi observar como o aumento do processamento melhora a
aproximação do valor de π. Para isso, foi usada a série de Leibniz com quatro
quantidades de iterações. O maior teste foi escolhido para levar aproximadamente
um segundo no computador utilizado.

## 2. Método utilizado

<div class="formula">π = 4 · (1 − 1/3 + 1/5 − 1/7 + 1/9 − ⋯)</div>

A série alterna uma soma e uma subtração. No programa, a variável `sinal`
começa com 1 e é multiplicada por −1 depois de cada termo. O denominador é
formado por `2 * i + 1`, produzindo os números ímpares 1, 3, 5, 7 e assim por
diante.

Foram testadas 10⁶, 10⁷, 10⁸ e 10⁹ iterações. Para cada teste, o programa
mostra a aproximação calculada, o erro absoluto em relação ao valor de referência
e o tempo medido com a função `clock()` da biblioteca padrão de C.

## 3. Resultados

O programa foi compilado com a opção de otimização `-O2`. Os tempos abaixo foram
obtidos em um Intel Core i7-13650HX com Windows 11 e podem variar um pouco entre
execuções.

| Iterações | Aproximação de π | Erro absoluto | Tempo (s) |
|---:|---:|---:|---:|
| 10⁶ | 3,141591653589774 | 1,000 × 10⁻⁶ | 0,0009 |
| 10⁷ | 3,141592553589792 | 1,000 × 10⁻⁷ | 0,0088 |
| 10⁸ | 3,141592643589326 | 1,000 × 10⁻⁸ | 0,0884 |
| 10⁹ | 3,141592652588050 | 1,002 × 10⁻⁹ | 0,8723 |

Quando o número de iterações foi multiplicado por 10, o tempo também aumentou
aproximadamente 10 vezes. Isso acontece porque cada nova iteração calcula um termo
da série. Ao mesmo tempo, o erro ficou aproximadamente 10 vezes menor, acrescentando
cerca de uma casa decimal correta ao resultado.

O teste com 10⁹ iterações levou 0,8723 segundo. Somando os testes menores, a
execução do cálculo ficou próxima de um segundo no computador utilizado.

## 4. Aplicações reais

Esse comportamento também aparece em simulações físicas. Usar mais pontos de
cálculo ou uma malha mais detalhada pode representar melhor fenômenos como o
movimento de fluidos e a previsão do tempo, mas aumenta o tempo de execução e o
uso de memória.

Na inteligência artificial, mais dados, épocas de treinamento ou modelos maiores
podem melhorar os resultados, mas exigem mais tempo e recursos computacionais. Nos
dois casos, pequenas melhorias de precisão podem exigir aumentos consideráveis de
processamento, assim como ocorreu na aproximação de π.

## 5. Conclusão

O experimento mostrou que aumentar o número de iterações melhora a aproximação de
π, mas também aumenta o tempo de execução. Na série de Leibniz, dez vezes mais
iterações produziram aproximadamente uma nova casa decimal correta e exigiram
aproximadamente dez vezes mais tempo. Portanto, existe uma relação direta entre a
quantidade de processamento, o tempo gasto e a precisão obtida.

<div class="quebra"></div>

## 6. Código-fonte

{{CODIGO:pi_serie.c}}
