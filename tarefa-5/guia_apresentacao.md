# Guia para apresentação — Tarefa 5

Este arquivo serve como apoio para a explicação presencial e não faz parte do
relatório entregue em PDF.

## Resumo

1. O programa verifica cada número de 2 até `n` por divisão de tentativa.
2. A versão sequencial produz o valor de referência.
3. Na segunda versão, somente `#pragma omp parallel for` foi acrescentado ao laço.
4. As iterações do teste de primalidade são independentes, mas o contador não é.
5. `quantidade++` pode perder incrementos por causa de uma condição de corrida.
6. A versão paralela pode ser mais rápida, embora seu resultado possa estar errado.
7. O custo das iterações varia, causando possível desequilíbrio de carga.
8. Os problemas podem não aparecer claramente em toda medição; isso não torna o
   programa correto nem perfeitamente balanceado.

## Como explicar o código

Siga esta ordem durante a apresentação:

1. **Função `eh_primo`:** verifica se um número é primo. Ela descarta valores
   menores que 2, trata o número 2 separadamente, elimina os outros pares e testa
   apenas divisores ímpares até a raiz quadrada.
2. **Função `contar_sequencial`:** percorre os números de 2 até `n` e incrementa
   `quantidade` sempre que encontra um primo.
3. **Função `contar_paralelo`:** executa o mesmo laço, acrescentando
   `#pragma omp parallel for` para dividir as iterações entre as threads.
4. **Problema principal:** o teste de cada número é independente, mas todas as
   threads modificam o mesmo contador com `quantidade++`. Isso pode causar perda
   de incrementos.
5. **Função `main`:** define o limite e a quantidade de threads, executa as duas
   versões, mede os tempos e apresenta os resultados.

### Fala curta sobre o código

> O programa possui uma função que verifica se um número é primo. Depois, ele
> percorre todos os números primeiro sequencialmente e depois em paralelo. A
> principal mudança na segunda versão é o `parallel for`. O teste de cada número
> pode ser dividido entre as threads, mas o contador é compartilhado, criando o
> problema observado.

### Partes auxiliares

- `walltime()` serve apenas para medir o tempo de execução;
- `ler_inteiro_positivo()` valida os argumentos informados pelo usuário;
- `omp_set_num_threads()` define quantas threads serão usadas;
- `numero / divisor` permite limitar os testes à raiz quadrada sem calcular a
  raiz e sem fazer uma multiplicação que possa ultrapassar o limite de `int`.

Não é necessário explicar cada `include`, toda a validação dos argumentos ou os
detalhes internos do relógio, a menos que o professor pergunte.

## Perguntas e respostas

### Como um número é classificado como primo?

Os divisores ímpares entre 3 e a raiz quadrada do número são testados. Se algum
deles dividir o número exatamente, ele é composto. O número 2 é tratado à parte e
os demais números pares são descartados imediatamente.

### Por que só é necessário testar até a raiz quadrada?

Se um número composto possui um divisor maior que sua raiz, o divisor
correspondente é menor que a raiz e já teria sido encontrado.

### O que mudou na versão paralela?

Foi acrescentado `#pragma omp parallel for` antes do mesmo laço. O OpenMP divide
as iterações entre várias threads.

### O teste de primalidade pode ser executado em paralelo?

Sim. A classificação de um número não depende da classificação dos demais. O
problema não está em `eh_primo`, mas na atualização do contador compartilhado.

### O que é uma condição de corrida?

É uma situação em que o resultado depende da ordem imprevisível dos acessos
simultâneos. Duas threads podem ler o mesmo valor de `quantidade`, incrementá-lo e
gravar o mesmo novo valor, perdendo um dos incrementos.

### Por que o erro pode mudar entre execuções?

O sistema operacional e o runtime do OpenMP podem intercalar as threads de formas
diferentes. Logo, a quantidade de incrementos sobrepostos também pode mudar.

### E se os resultados forem iguais em uma execução?

Isso não prova que o programa está correto. A corrida continua existindo; apenas
pode não ter produzido um erro visível naquela execução específica.

### Como o problema de correção poderia ser resolvido?

Uma solução natural seria declarar uma redução:

```c
#pragma omp parallel for reduction(+:quantidade)
```

Cada thread teria um contador privado e o OpenMP somaria os valores ao final. A
solução é explicada, mas não foi aplicada ao experimento principal porque o
objetivo é observar o problema após paralelizar diretamente o laço original.

### Por que existe possível desequilíbrio de carga?

As iterações não têm o mesmo custo. Um número par termina quase imediatamente;
um composto ímpar termina quando encontra seu primeiro divisor; um primo percorre
todos os divisores possíveis até sua raiz quadrada. Números maiores também podem
exigir mais testes.

### O que acontece quando a carga fica desequilibrada?

Algumas threads acabam seu bloco e ficam ociosas enquanto outras ainda trabalham.
O tempo do laço é determinado pela última thread a terminar, limitando o speedup.

### Como a distribuição poderia ser melhorada?

Um escalonamento dinâmico, como `schedule(dynamic)`, pode entregar novos grupos de
iterações às threads que terminarem antes. Porém, ele também possui overhead e não
foi necessário para demonstrar o problema pedido.

### Ser mais rápido significa que o programa paralelo está correto?

Não. Desempenho e correção são propriedades diferentes. O tempo só deve ser
comparado como solução útil depois de validar que os resultados são equivalentes.
