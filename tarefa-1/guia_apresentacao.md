# Guia para apresentação — Tarefa 1

Este arquivo serve como apoio para a explicação presencial. Ele não faz parte do
relatório entregue em PDF.

## Resumo para apresentação

1. Foi utilizada a série de Leibniz, que aproxima π por uma soma infinita com
   sinais alternados.
2. Como não é possível calcular infinitos termos, o programa interrompe a série
   depois de uma quantidade determinada de iterações.
3. A variável `sinal` alterna entre 1 e −1, enquanto `2 * i + 1` produz os
   denominadores ímpares.
4. Foram testadas quantidades de 10⁶ até 10⁹ iterações. Em cada teste, o programa
   mediu o tempo e calculou o erro em relação ao valor de referência de π.
5. Quando as iterações foram multiplicadas por dez, o erro ficou aproximadamente
   dez vezes menor, mas o tempo também aumentou aproximadamente dez vezes.
6. Com 10⁹ iterações, o tempo ficou próximo de um segundo no computador utilizado.
7. O comportamento também aparece em simulações físicas e inteligência artificial:
   mais processamento pode melhorar o resultado, mas aumenta o tempo e o uso de
   recursos.

## Perguntas e respostas

### O que é erro absoluto?

É a diferença positiva entre o valor calculado e o valor de referência. No código,
ele é calculado com `fabs(PI_REAL - pi)`.

### Para que serve `clock()`?

A função `clock()` mede o tempo de processamento utilizado pelo programa. A
divisão por `CLOCKS_PER_SEC` converte o valor medido para segundos.

### Por que foi usado o tipo `double`?

Porque π possui casas decimais e o tipo `double` oferece precisão suficiente para
este experimento.

### Por que o tempo aumenta aproximadamente dez vezes?

Porque multiplicar o número de iterações por dez faz o laço executar
aproximadamente dez vezes mais operações.

### Por que o erro diminui quando há mais iterações?

A série de Leibniz possui infinitos termos. Ao calcular mais termos, o programa
interrompe a série mais perto de seu resultado final, que é π.

### Por que foram usadas até 10⁹ iterações?

Essa quantidade fez o maior teste ficar próximo de um segundo no computador
utilizado, conforme a orientação dada para a tarefa.

### O programa é paralelo?

Não. O enunciado pede para observar o efeito de aumentar o processamento, mas não
exige paralelização. Este programa é um experimento sequencial e pode servir como
base para uma futura comparação com uma versão paralela.

### O que a opção `-O2` faz?

Ela solicita que o compilador otimize o programa. Como a tarefa mede tempo, é
importante utilizar sempre a mesma opção de compilação para que os resultados
possam ser comparados.

### Como esse comportamento aparece em simulações físicas?

Uma simulação pode usar mais pontos de cálculo ou uma malha mais detalhada para
representar melhor um fenômeno. Isso pode aumentar a precisão, mas também aumenta
o tempo de execução e o uso de memória.

### Como esse comportamento aparece na inteligência artificial?

O treinamento pode utilizar mais dados, mais épocas ou modelos maiores para tentar
melhorar os resultados. Essas melhorias normalmente exigem mais tempo e recursos
computacionais.

### Qual valor de π foi usado como referência?

Foi utilizado `3.141592653589793`, definido no código como `PI_REAL`.
