# Critical, atomic, privatização e reduction no OpenMP

## 1. Objetivo

O experimento estima π pelo método de Monte Carlo. Para cada um dos `N` pontos,
duas coordenadas pseudoaleatórias são geradas no quadrado unitário; se
`x² + y² <= 1`, ocorre um *hit*. Ao final:

<div class="formula">π ≈ 4 × hits / N</div>

O estudo compara cinco maneiras corretas de acumular os hits em paralelo:

| Versão | Contagem durante o laço | Combinação global explícita |
|---|---|---:|
| compartilhado + `critical` | contador global | uma por hit |
| compartilhado + `atomic` | contador global | uma por hit |
| privado + `critical` | contador de cada thread | uma por thread |
| privado + `atomic` | contador de cada thread | uma por thread |
| `reduction` | cópia privada administrada pelo OpenMP | nenhuma no código |

Todas as versões usam `rand_r`, sementes privadas por thread e
`schedule(static)`. Com a mesma semente e equipe, cada thread gera a mesma
sequência nas cinco versões. Assim, diferenças de resultado indicariam erro de
sincronização, enquanto diferenças de tempo refletem a estratégia de acumulação.

## 2. Por que `rand_r` e estado privado

`rand()` mantém estado global oculto. Chamá-lo concorrentemente pode exigir
sincronização interna ou causar uma corrida, conforme a implementação. `rand_r`
recebe o endereço do estado que deve atualizar. Cada thread mantém sua variável
`estado` na pilha:

```c
int id = omp_get_thread_num();
unsigned int estado = semente_da_thread(semente, id);
double x = (double)rand_r(&estado) / (double)RAND_MAX;
```

Portanto, threads diferentes não escrevem no mesmo estado do gerador. No Windows,
onde a UCRT não oferece a extensão POSIX `rand_r`, o fonte inclui uma implementação
compatível para permitir o mesmo experimento. O objetivo não é avaliar a qualidade
estatística do gerador, mas isolar os mecanismos OpenMP.

## 3. Contador compartilhado com `critical`

Na primeira versão, cada hit entra em uma região crítica e incrementa o contador:

```c
if (ponto_dentro_do_circulo(&estado)) {
    #pragma omp critical
    {
        hits_compartilhados++;
    }
}
```

`critical` aceita um bloco arbitrário e garante que apenas uma thread por vez
execute uma região com o mesmo nome. É uma ferramenta geral: poderia proteger
várias instruções, campos relacionados ou uma operação sobre uma estrutura. Aqui,
porém, o bloco possui apenas um incremento e é executado em aproximadamente 78,5%
das iterações. Milhões de aquisições serializam a parte mais frequente do laço e
produzem grande contenção.

## 4. Contador compartilhado com `atomic`

A segunda versão preserva o contador global, mas protege somente a atualização:

```c
if (ponto_dentro_do_circulo(&estado)) {
    #pragma omp atomic update
    hits_compartilhados++;
}
```

`atomic` não protege um bloco geral. Ele se aplica a formas de acesso reconhecidas
pelo OpenMP, como `x++`, `x += expressao` e operações de leitura, escrita, captura
ou comparação admitidas pela versão da especificação. O compilador normalmente
pode gerar uma operação atômica de hardware ou uma sequência especializada, sem
o protocolo mais geral de entrada e saída de uma região crítica. Por isso tende a
ser mais barato que `critical` para um único contador.

Atômico não significa paralelo sobre a mesma posição: as atualizações ainda
precisam ser ordenadas pelo sistema de coerência de cache. Logo, o contador
continua sendo um ponto de contenção e o custo cresce com a frequência de hits e
o número de threads.

## 5. Privatização seguida de combinação

As versões seguintes removem o contador global do caminho quente. Cada thread
incrementa uma variável local, que normalmente permanece em registrador, e só
combina seu subtotal depois do `omp for`:

```c
long long hits_privados = 0;

#pragma omp for schedule(static)
for (long long i = 0; i < pontos; i++) {
    hits_privados += ponto_dentro_do_circulo(&estado);
}

#pragma omp atomic update
hits_compartilhados += hits_privados;
```

Há uma versão com `critical` e outra com `atomic` na soma final. Com `T` threads,
cada thread executa uma atualização global, totalizando `T`, em vez de
aproximadamente `0,785 × N`. No teste, isso reduz cerca de 15,7 milhões de
sincronizações para apenas oito. Como a frequência domina o custo, a escolha entre
`critical` e `atomic` na combinação final quase desaparece nas medições.

Essa transformação exige que a operação admita subtotais independentes e uma
combinação associativa. Também acrescenta código manual, criando oportunidades
para esquecer a inicialização, a combinação ou a proteção final.

## 6. Versão com `reduction`

`reduction(+:hits)` expressa diretamente o mesmo padrão de privatizar e combinar:

```c
long long hits = 0;

#pragma omp parallel default(none) shared(pontos, semente) \
    reduction(+:hits)
{
    /* estado de rand_r privado por thread */
    #pragma omp for schedule(static)
    for (long long i = 0; i < pontos; i++) {
        hits += ponto_dentro_do_circulo(&estado);
    }
}
```

O runtime cria uma cópia privada de `hits` para cada thread, inicializa-a com o
elemento neutro da soma e combina as cópias ao final. A tabela do programa mostra
zero atualizações **explicitamente** sincronizadas porque não há `critical` nem
`atomic` no fonte; ainda existe uma etapa interna de redução.

Quando a operação e o tipo são suportados, `reduction` oferece simultaneamente
boa eficiência e alta produtividade: o código é menor, documenta a intenção e
delega ao compilador/runtime a estratégia de combinação.

## 7. Metodologia e validação

O programa foi compilado sem avisos com:

```text
gcc -O2 -Wall -Wextra -std=c11 -fopenmp pi_sincronizacao.c -o pi_sincronizacao.exe
```

Antes da medição, uma região paralela de aquecimento cria a equipe. Cada versão
processa 20 milhões de pontos com oito threads. Foram realizadas cinco repetições
com sementes consecutivas; o tempo é de parede, medido por `omp_get_wtime`.

Em cada repetição, as cinco versões produziram exatamente o mesmo número de hits
e o programa imprimiu `Validacao: OK`. Os valores de π variaram aproximadamente
entre 3,14150 e 3,14173, comportamento esperado de uma estimativa estocástica.

## 8. Resultados

| Versão | Atualizações explícitas por execução | Tempos (s) | Mediana (s) | Razão contra `critical` compartilhado |
|---|---:|---|---:|---:|
| compartilhado + `critical` | ≈ 15,7 milhões | 1,086; 1,122; 1,088; 1,074; 1,098 | 1,088 | 1,0× |
| compartilhado + `atomic` | ≈ 15,7 milhões | 0,190; 0,190; 0,191; 0,196; 0,188 | 0,190 | 5,7× |
| privado + `critical` | 8 | 0,020; 0,018; 0,019; 0,021; 0,018 | 0,019 | 57,3× |
| privado + `atomic` | 8 | 0,018; 0,019; 0,019; 0,018; 0,020 | 0,019 | 57,3× |
| `reduction` | 0 no fonte | 0,018; 0,019; 0,018; 0,019; 0,019 | 0,019 | 57,3× |

O `atomic` compartilhado foi aproximadamente 5,7 vezes mais rápido que o
`critical` compartilhado porque sua operação é mais restrita e pode ser
implementada de modo especializado. Entretanto, privatizar foi muito mais
importante do que trocar a primitiva: as versões privadas e a redução foram cerca
de 57 vezes mais rápidas que a primeira versão.

As diferenças de um milissegundo entre as três melhores versões são pequenas e
não estabelecem uma vencedora universal. Nesse tamanho de teste, todas gastam
quase todo o tempo gerando e classificando pontos; somente oito combinações finais
têm custo desprezível. A redução é preferível aqui por expressar a intenção com
menos código, mesmo sem ter a menor mediana isolada.

## 9. Desempenho, produtividade e roteiro de decisão

Uma estratégia prática é procurar primeiro uma formulação que evite estado
compartilhado frequente e somente depois escolher a primitiva:

1. **A operação é uma soma, produto, mínimo, máximo, operação lógica ou outra
   redução suportada?** Usar `reduction`. É concisa, difícil de usar
   incorretamente e normalmente escalável.
2. **É possível acumular privadamente e combinar uma vez por thread/tarefa?**
   Privatizar, mesmo quando a combinação final precisar de `atomic` ou `critical`.
   Reduzir a frequência de sincronização costuma valer mais que trocar de
   primitiva.
3. **Resta uma atualização simples de uma única posição?** Usar `atomic`. É leve,
   mas apenas quando a forma aceita pelo OpenMP preserva todo o invariante.
4. **É necessário proteger várias instruções ou vários campos como uma única
   operação?** Usar `critical`. É simples e legível, porém serializa o bloco.
5. **Há poucos recursos independentes conhecidos na compilação?** Usar regiões
   críticas nomeadas, por exemplo `critical(fila_a)` e `critical(fila_b)`, para
   que recursos diferentes não se bloqueiem desnecessariamente.
6. **A quantidade de recursos é dinâmica, é preciso escolher a proteção por
   índice, tentar adquirir sem bloquear ou controlar o ciclo de vida?** Usar
   locks explícitos (`omp_init_lock`, `omp_set_lock`, `omp_unset_lock` e
   `omp_destroy_lock`). Eles oferecem mais controle, mas exigem mais código e
   cuidado com esquecimento de liberação, ordem de aquisição, deadlock e falso
   compartilhamento entre locks vizinhos.

Nenhuma ferramenta é sempre mais rápida. A região protegida, a frequência, a
contenção e a arquitetura determinam o resultado. Em produtividade, `reduction`
é a melhor abstração para este problema; `atomic` é a solução mínima para uma
atualização simples; `critical` favorece clareza em invariantes maiores; e locks
explícitos ficam reservados para estruturas que realmente precisam do controle
dinâmico adicional.

## 10. Conclusão

As cinco versões são corretas, mas expõem custos muito diferentes. Substituir
`critical` por `atomic` reduz o custo de cada incremento, sem eliminar a contenção
no contador compartilhado. Privatizar altera a escala do problema: o número de
operações sincronizadas deixa de ser proporcional aos hits e passa a ser
proporcional às threads. `reduction` realiza essa privatização com a descrição
mais curta e direta, sendo a escolha natural para a soma de hits.

<div class="quebra"></div>

## 11. Código-fonte completo

{{CODIGO:pi_sincronizacao.c}}
