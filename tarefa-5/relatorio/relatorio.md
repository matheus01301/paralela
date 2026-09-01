# Contagem sequencial e paralela de números primos

## Resumo para a defesa

- **Objetivo:** contar os números primos entre 2 e `n` e comparar as execuções
  sequencial e paralela.
- **O que foi paralelizado:** o laço principal recebeu a diretiva
  `#pragma omp parallel for`.
- **O que pode ser executado independentemente:** o teste de primalidade de cada
  número.
- **Problema de correção:** todas as threads atualizam o mesmo contador
  `quantidade`, podendo perder incrementos.
- **Problema de distribuição:** testar alguns números custa mais do que testar
  outros, portanto algumas threads podem terminar antes das demais.
- **Resultado principal:** a versão sequencial encontrou 664.579 primos; a
  paralela encontrou cerca de 31 mil, mostrando que ficou mais rápida, mas
  incorreta.
- **Conclusão:** adicionar uma diretiva de paralelização não garante, por si só,
  um programa correto nem uma divisão equilibrada do trabalho.

**Frase-chave:** o teste de cada número é independente, mas o contador usado
para juntar os resultados é compartilhado.

## 1. Objetivo

O objetivo desta tarefa foi implementar em C um programa que conta quantos
números primos existem entre 2 e um limite `n`. Depois, o laço principal foi
paralelizado diretamente com `#pragma omp parallel for`, sem modificar a lógica
do contador. As versões sequencial e paralela foram comparadas quanto ao resultado
e ao tempo de execução.

O experimento também introduz dois desafios da programação paralela: manter a
correção quando várias threads alteram um mesmo dado e distribuir de forma
equilibrada iterações que possuem custos diferentes.

## 2. Implementação

### 2.1 Teste de primalidade

A função `eh_primo` trata o número 2 separadamente, elimina os demais números
pares e testa somente divisores ímpares. Os divisores são verificados até a raiz
quadrada do número. A condição foi escrita como `divisor <= numero / divisor`
para evitar uma multiplicação que poderia ultrapassar o limite do tipo `int`.

Não é necessário testar divisores maiores que a raiz quadrada: se um número
composto possuir um fator maior que sua raiz, o fator correspondente será menor
que a raiz e já terá sido testado.

### 2.2 Versão sequencial

Na versão sequencial, cada número entre 2 e `n` é testado e a variável
`quantidade` é incrementada quando um primo é encontrado:

```c
for (numero = 2; numero <= n; numero++) {
    if (eh_primo(numero)) {
        quantidade++;
    }
}
```

Como existe apenas uma thread, todos os incrementos são executados em sequência e
o contador fornece o resultado de referência.

### 2.3 Versão paralela

A lógica original foi mantida e somente a diretiva solicitada foi acrescentada:

```c
#pragma omp parallel for
for (numero = 2; numero <= n; numero++) {
    if (eh_primo(numero)) {
        quantidade++;
    }
}
```

As chamadas de `eh_primo` são independentes: descobrir se um número é primo não
depende do resultado dos outros números. Entretanto, `quantidade` é compartilhada
por todas as threads, o que cria um problema de correção discutido na Seção 5.

## 3. Método de medição

O programa foi compilado com:

```text
gcc -O2 -Wall -Wextra -std=c99 -fopenmp primos.c -o primos.exe
```

Foi usado `n = 10.000.000` e a versão paralela executou com 20 threads, quantidade
de processadores lógicos disponível no Intel Core i7-13650HX da máquina de teste.
O tempo de parede foi medido por `QueryPerformanceCounter` no Windows e por
`clock_gettime(CLOCK_MONOTONIC)` em sistemas POSIX. O programa completo foi
executado cinco vezes pelo script `executar_testes.ps1`.

O uso de processos separados nas repetições permite observar variações no
escalonamento e evita que uma única execução seja tratada como resultado absoluto.
O valor central dos tempos, a mediana, foi usado no resumo.

## 4. Resultados

| Execução | Primos sequencial | Tempo sequencial (s) | Primos paralelo | Tempo paralelo (s) | Speedup informado |
|---:|---:|---:|---:|---:|---:|
| 1 | 664.579 | 2,294497 | 31.001 | 0,325218 | 7,055× |
| 2 | 664.579 | 2,300537 | 31.001 | 0,306187 | 7,513× |
| 3 | 664.579 | 2,259462 | 31.001 | 0,302109 | 7,479× |
| 4 | 664.579 | 2,264279 | 31.001 | 0,302008 | 7,497× |
| 5 | 664.579 | 2,259386 | 31.089 | 0,314836 | 7,176× |
| **Mediana do tempo** | **664.579** | **2,264279** | — | **0,306187** | **7,395×** |

A versão sequencial sempre encontrou 664.579 primos, que é a contagem esperada
até dez milhões. A versão paralela retornou 31.001 ou 31.089, dependendo da
execução. Portanto, ela perdeu mais de 633 mil incrementos.

O tempo paralelo mediano foi menor, passando de 2,264279 s para 0,306187 s. A
razão entre esses tempos é 7,395×. Contudo, esse valor não representa o *speedup*
de uma solução correta: a execução paralela fez os testes de primalidade, mas
falhou ao consolidar seus resultados. Desempenho só deve ser comparado como
resultado útil depois que a equivalência das saídas for validada.

## 5. Desafio de correção

Pontos principais:

- `quantidade` é compartilhada entre as threads;
- `quantidade++` envolve leitura, soma e escrita;
- duas threads podem ler o mesmo valor e gravar o mesmo resultado;
- quando isso acontece, um dos incrementos é perdido;
- por isso, o resultado paralelo pode estar errado e variar entre execuções.

A expressão `quantidade++` parece uma única operação no código-fonte, mas envolve
ler o valor atual, somar um e gravar o novo valor. Duas threads podem ler o mesmo
valor antes que qualquer uma grave sua atualização. As duas então gravam o mesmo
novo valor e um incremento é perdido.

Esse acesso concorrente sem sincronização é uma condição de corrida. Além dos
incrementos perdidos observados, uma corrida de dados torna o comportamento do
programa indefinido segundo o modelo de memória. A saída pode depender da ordem
imprevisível de execução das threads e das decisões do compilador.

Por isso, obter resultados iguais em uma execução não provaria que o programa é
correto. Da mesma forma, o fato de o problema aparecer claramente nesta máquina
não garante que ele apareça com a mesma intensidade em outro ambiente.

### 5.1 Bônus: uma possível correção com `reduction`

Como conhecimento adicional, uma forma apropriada de corrigir o contador seria
usar:

```c
#pragma omp parallel for reduction(+:quantidade)
```

Com `reduction`, cada thread acumularia uma quantidade privada e o OpenMP somaria
os valores ao final. Esse recurso não foi aplicado à versão medida: ele é
apresentado apenas como uma extensão e uma possível solução para o problema
observado.

## 6. Desafio de distribuição de carga

Pontos principais:

- um número par é descartado quase imediatamente;
- um número composto ímpar termina quando seu primeiro divisor é encontrado;
- um número primo testa todos os divisores ímpares até sua raiz quadrada.
- algumas threads podem terminar cedo e ficar ociosas;
- o tempo total depende da última thread que terminar seu trabalho.

Além disso, a raiz quadrada cresce com o valor do candidato. Assim, as faixas
próximas de `n` tendem a permitir mais testes que as faixas iniciais. Dependendo de
como o runtime distribui os blocos, algumas threads podem terminar cedo e ficar
ociosas enquanto outras continuam processando iterações mais caras. Como o laço
só termina quando a última thread conclui seu trabalho, esse desequilíbrio limita
o ganho de desempenho.

É possível que o desequilíbrio não apareça de maneira evidente nos tempos de
uma execução. Sua ausência visível não significa que todas as iterações tenham o
mesmo custo.

### 6.1 Bônus: uma possível melhoria com `schedule(dynamic)`

Como conhecimento adicional, `schedule(dynamic)` poderia entregar novos blocos
de iterações às threads que terminassem primeiro. Isso pode melhorar o equilíbrio,
mas também introduz um custo de gerenciamento. O recurso não foi usado no
experimento; ele é apresentado somente como uma possível melhoria.

## 7. Conclusão

A inclusão direta de `#pragma omp parallel for` reduziu o tempo observado, mas não
produziu uma versão correta. O contador compartilhado sofreu uma condição de
corrida e a resposta paralela variou entre execuções, enquanto a sequencial
permaneceu em 664.579 primos.

O experimento mostra que paralelizar um laço não consiste apenas em dividi-lo
entre threads. Primeiro é preciso verificar quais dados são independentes e quais
são compartilhados. Depois, também é necessário considerar se as iterações têm
custos semelhantes, pois uma divisão numericamente igual pode não representar uma
divisão igual de trabalho.

Os dois problemas são potenciais e dependem da execução para se manifestarem no
resultado ou no tempo. Mesmo quando uma medição não exibe erro ou grande
desequilíbrio, a análise do código continua necessária para avaliar correção e
desempenho.

<div class="quebra"></div>

## 8. Código-fonte

{{CODIGO:primos.c}}
