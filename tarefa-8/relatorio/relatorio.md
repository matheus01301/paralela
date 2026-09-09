# rand(), rand_r() e falso compartilhamento na estimativa de π

## 1. Objetivo e método

O objetivo foi estimar π por Monte Carlo e comparar quatro implementações
OpenMP, combinando dois geradores pseudoaleatórios e dois métodos de acumulação.
Cada amostra é um ponto uniforme `(x,y)` no quadrado `[0,1] × [0,1]`. O ponto é
um acerto quando `x² + y² ≤ 1`; logo:

<div class="formula">π̂ = 4 · acertos / número total de pontos</div>

As versões foram executadas com a mesma quantidade de pontos, número de threads
e semente. O tempo de parede foi medido com `omp_get_wtime()`. A soma serial final
do vetor faz parte do intervalo cronometrado; somente a inicialização e a
impressão ficaram de fora. A soma possui apenas uma parcela por thread e custo
desprezível diante dos dez milhões de amostras.

## 2. As quatro versões

### 2.1 `rand()` com contador privado e `critical`

Cada thread conta os acertos em uma variável local. No fim, entra uma única vez
na região crítica para acrescentar seu subtotal à variável global:

```c
#pragma omp parallel default(none) shared(pontos, total_global)
{
    long long acertos_locais = 0;

    #pragma omp for schedule(static)
    for (long long i = 0; i < pontos; i++) {
        double x = coordenada(rand());
        double y = coordenada(rand());
        acertos_locais += (x * x + y * y <= 1.0);
    }

    #pragma omp critical
    total_global += acertos_locais;
}
```

O contador local normalmente permanece em registrador ou cache privado. Como há
apenas uma entrada no `critical` por thread, o custo dessa acumulação é O(T),
onde T é o número de threads, e não O(N) para N pontos.

### 2.2 `rand()` com vetor compartilhado

Cada thread incrementa `acertos_por_thread[id]`. Não existe condição de corrida,
pois cada posição tem um único escritor. Depois da região paralela, a thread
principal percorre o vetor e produz o total.

As posições, entretanto, são `long long` contíguos. Em uma máquina com linhas de
cache de 64 bytes, oito contadores de 8 bytes ocupam a mesma linha. O vetor foi
declarado `volatile` para impedir que os incrementos fossem promovidos para um
registrador e, assim, preservar no benchmark os acessos à memória necessários
para observar o falso compartilhamento. `volatile` não substitui sincronização;
a correção decorre exclusivamente de haver um escritor por posição.

### 2.3 e 2.4 Substituição por `rand_r()`

Nas duas últimas versões, cada thread mantém uma semente privada e passa seu
endereço a `rand_r()`:

```c
int id = omp_get_thread_num();
unsigned int estado = semente + 0x9e3779b9u * (unsigned int)(id + 1);
double x = coordenada(rand_r(&estado));
double y = coordenada(rand_r(&estado));
```

As sementes iniciais são diferentes para que as threads não percorram sequências
iguais. Com `schedule(static)`, cada thread processa sempre o mesmo número de
pontos e, portanto, as duas versões `rand_r()` produzem exatamente a mesma
contagem. No ambiente POSIX foi usada a implementação da glibc. O fonte inclui
uma pequena compatibilidade para a UCRT/MinGW, onde `rand_r()` não existe, mas as
medições deste relatório foram feitas no Linux/WSL.

## 3. Coerência de cache e falso compartilhamento

Cache coerente significa que os núcleos devem concordar sobre o valor de uma
linha. Para escrever nela, um núcleo obtém sua propriedade exclusiva e invalida
as cópias presentes nos outros caches. Isso é necessário mesmo quando os núcleos
alteram palavras diferentes da linha, pois o protocolo opera na granularidade da
linha inteira, e não de cada variável.

No contador privado, os incrementos não geram tráfego de coerência. Somente os T
subtotais atravessam o `critical` e atualizam `total_global`, ocasionando poucas
transferências de propriedade da linha. No vetor contíguo, grupos de oito
threads escrevem milhares de vezes em contadores diferentes da mesma linha. A
linha “salta” entre os caches, suas cópias são invalidadas repetidamente e os
escritores esperam pela propriedade exclusiva. Esse efeito é falso
compartilhamento: os dados são logicamente independentes, porém compartilham a
unidade física de coerência.

`rand()` introduz outro compartilhamento. Na glibc 2.39 usada no teste, seu estado
global é protegido para chamadas concorrentes. As threads disputam essa
proteção em cada coordenada — duas vezes por ponto —, serializando o gerador e
causando tráfego de coerência em seu estado e em sua trava. Assim, nas duas
primeiras versões, o custo de `rand()` domina e esconde parte da diferença entre
os métodos de acumulação.

`rand_r()` elimina esse gargalo porque o estado apontado por `estado` é privado.
O gerador escala entre os núcleos; por isso, o método de acumulação passa a ser
muito mais visível. A versão `rand_r + critical` combina estado aleatório e
contador privados durante o laço e só compartilha uma atualização por thread.
Já `rand_r + vetor` remove a trava lógica, mas sofre invalidações da linha a cada
acerto. Neste caso, evitar `critical` não garante maior desempenho.

Também é importante não generalizar o resultado de `rand()` para toda biblioteca
C: o padrão C não oferece `rand_r()` e as escolhas de segurança e estado de
`rand()` variam entre implementações. A conclusão quantitativa vale para a glibc
medida; a conclusão sobre o vetor contíguo decorre da organização da cache.

## 4. Resultados

O programa foi compilado no Ubuntu 24.04/WSL 2 com GCC 13.3.0, glibc 2.39 e:

```text
gcc -O2 -Wall -Wextra -std=c11 -fopenmp pi_rand_openmp.c -o pi_rand_openmp
```

Foram realizadas cinco execuções com 10.000.000 de pontos, 16 threads e semente
2026 em um AMD Ryzen 7 5700X3D (8 núcleos, 16 threads lógicas). A tabela apresenta
a mediana dos tempos:

| Versão | π (versão `rand_r`) | Mediana | Em relação a `rand_r + critical` |
|---|---:|---:|---:|
| `rand + critical` | variável entre execuções | 0,839048 s | 60,81× mais lenta |
| `rand + vetor` | variável entre execuções | 0,947555 s | 68,68× mais lenta |
| `rand_r + critical` | 3,141424400 | 0,013797 s | referência |
| `rand_r + vetor` | 3,141424400 | 0,038935 s | 2,82× mais lenta |

Com `rand()`, o vetor foi 12,93% mais lento que o contador privado. Com
`rand_r()`, a penalidade do vetor aumentou para 182,20%. Isso não significa que
`critical` seja intrinsecamente rápido: ele é barato aqui porque aparece uma vez
por thread. Se estivesse dentro do laço, haveria milhões de operações
serializadas.

As estimativas de π são plausíveis e próximas do valor real. As versões `rand()`
variaram mesmo após `srand(2026)`: a ordem concorrente das chamadas decide quais
números consecutivos do estado global formam cada par `(x,y)`. Nas versões
`rand_r()`, cada fluxo pertence a uma thread e o escalonamento estático tornou o
resultado reprodutível: 7.853.561 acertos nas cinco repetições.

Os valores completos de todas as execuções estão em `resultados.txt`. Tempos
muito curtos variam com carga do sistema, frequência do processador e runtime;
por isso foi usada a mediana, não uma única observação.

## 5. Conclusão

A versão mais rápida foi `rand_r + critical`. Seu desempenho resulta de dois
tipos de privacidade: cada thread mantém tanto o estado do gerador quanto o
contador durante o laço, compartilhando somente o subtotal final. `rand()` foi o
principal gargalo nas versões que o utilizaram, devido à serialização do estado
global da glibc. No vetor, posições distintas evitaram corrida, mas não evitaram
falso compartilhamento; a coerência trabalha com linhas inteiras e tornou a
solução sem `critical` mais lenta. Separar cada contador por uma linha de cache
(*padding*) ou acumular localmente e gravar no vetor apenas uma vez eliminaria
grande parte dessa penalidade, embora mudasse o experimento solicitado.

<div class="quebra"></div>

## 6. Código-fonte completo

{{CODIGO:pi_rand_openmp.c}}
