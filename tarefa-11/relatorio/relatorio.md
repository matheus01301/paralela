# Difusão viscosa com diferenças finitas e OpenMP

## O modelo

O enunciado pede só o efeito da viscosidade. Tirando advecção, pressão e forças
externas de Navier-Stokes, cada componente da velocidade obedece a uma equação
de difusão, e as duas ficam independentes:

<div class="formula">∂u/∂t = ν ∇²u &nbsp;&nbsp;&nbsp;&nbsp; ∂v/∂t = ν ∇²v</div>

O domínio é `[0,1] × [0,1]` em uma malha `n × n` com `h = 1/n`. As derivadas
segundas usam diferenças centrais e o tempo avança por Euler explícito:

<div class="formula">∇²u ≈ (u<sub>dir</sub> + u<sub>esq</sub> + u<sub>cima</sub> + u<sub>baixo</sub> − 4·u<sub>centro</sub>) / h² &nbsp;&nbsp;&nbsp;&nbsp; u<sup>novo</sup> = u<sup>atual</sup> + ν·Δt·∇²u<sup>atual</sup></div>

O laplaciano compara a célula com a vizinhança. Se ela está acima, o valor cai;
se está abaixo, sobe. É isso que espalha uma perturbação e uniformiza o campo.

Euler explícito em duas dimensões só é estável se `r = ν·Δt/h² ≤ 1/4`. O programa
define `Δt` a partir de `r = 0,20`. A leitura prática: mais viscosidade significa
difusão mais rápida e passo de tempo **menor** — aumentar `ν` não permite
aumentar `Δt`.

## Como o código está montado

**Quatro matrizes.** `u`, `v` são o estado atual; `un`, `vn` o próximo. O novo
valor é calculado lendo só o estado atual. Atualizar no mesmo vetor faria o
resultado depender da ordem de varredura, porque uma célula já atualizada
entraria como vizinha da seguinte — e em paralelo seria condição de corrida. Ao
fim do passo só os ponteiros são trocados, sem cópia de dados.

**Bordas periódicas.** O vizinho da última coluna é a primeira, via resto da
divisão. Uma parede seria uma condição de contorno que o enunciado não pede, e
faria o campo constante mudar justamente na borda, estragando a validação. Em
troca ganha-se um teste: com o contorno fechado sobre si mesmo, a soma de todas
as células tem que se conservar.

Como cada célula do passo novo depende apenas de valores que ninguém altera
durante o passo, não há dependência entre células. É por isso que o laço
paraleliza sem `critical`, `atomic` ou `reduction`.

## Validação

Malha 1024 × 1024, 200 passos, sequencial.

| Teste | Esperado | Obtido |
|---|---|---|
| fluido parado (`u=0, v=0`) | continua zero | maior `|u|` = 0,000e+00 |
| campo constante (`u=2, v=−1`) | não muda | desvio = 0,000e+00 |
| perturbação central | pico cai e difunde | 2,000000 → 1,001986 |
| soma total | conservada | 1048577,0000 → 1048577,0000 (erro 1,5e−08) |
| `r = 0,30`, acima de 1/4 | deve divergir | pico final 3,1e+26 |

Os dois primeiros dão zero **exato**, não aproximado: num campo uniforme a conta
`4x − 4x` é exatamente zero em ponto flutuante. O erro de 1,5e−08 sobre uma soma
de 1,05e+06 é arredondamento (1,5e−14 relativo), não perda de massa. E o último
teste confirma o limite de estabilidade pelo lado de fora.

## Paralelização

São sete funções com **exatamente o mesmo corpo**; só a diretiva muda. O
experimento só mede o que promete se a diretiva for a única variável.

```c
static void passo_static(const Malha *m, const double *u, const double *v,
                         double *un, double *vn)
{
#pragma omp parallel for schedule(static)
    for (int i = 0; i < m->n; i++)
        for (int j = 0; j < m->n; j++)
            atualiza_celula(m, u, v, un, vn, i, j);
}
```

As seis diretivas comparadas:

```c
#pragma omp parallel for schedule(static)
#pragma omp parallel for collapse(2) schedule(static)
#pragma omp parallel for schedule(dynamic)
#pragma omp parallel for collapse(2) schedule(dynamic)
#pragma omp parallel for schedule(guided)
#pragma omp parallel for collapse(2) schedule(guided)
```

Nenhuma recebeu tamanho de bloco explícito, então vale o padrão: bloco 1 para
`dynamic`, bloco decrescente para `guided`. Esse detalhe explica a maior parte
dos resultados. `collapse(2)` funde os dois laços num espaço de 1.048.576
iterações em vez de 1.024 — não muda a quantidade de contas, só o que o OpenMP
tem para distribuir.

As seis versões produziram o campo final idêntico **byte a byte** ao sequencial
(`memcmp`, sem tolerância), em todas as contagens de thread.

## Ambiente e resultados

NPAD/UFRN, partição `amd-512`, nó `r2n00` com `--exclusive`: 2 sockets × 64
núcleos, 2 threads por núcleo, 512 GB. gcc 8.5.0 com
`-O2 -Wall -Wextra -std=c11 -fopenmp`, `OMP_PLACES=cores`, `OMP_PROC_BIND=close`.
Tempo por `clock_gettime(CLOCK_MONOTONIC)`, mediana de 5 execuções.

O nó exclusivo e a fixação das threads existem para que a diretiva seja a única
coisa que varia entre as medições. O tempo sequencial saiu 0,6973, 0,6977 e
0,6974 s nas três rodadas, o que confirma a estabilidade da medida.

Malha 1024 × 1024, 200 passos. Tempo em segundos, speedup entre parênteses.

| schedule | collapse | 16 threads | 32 threads | 64 threads |
|---|---|---:|---:|---:|
| sequencial | — | 0,6973 | 0,6977 | 0,6974 |
| `static` | não | **0,0567** (12,3×) | **0,0315** (22,1×) | **0,0218** (32,1×) |
| `static` | `collapse(2)` | 0,1068 (6,5×) | 0,0558 (12,5×) | 0,0323 (21,6×) |
| `dynamic` | não | 0,2290 (3,0×) | 0,2183 (3,2×) | 0,2866 (2,4×) |
| `dynamic` | `collapse(2)` | 7,4154 (0,09×) | 6,9383 (0,10×) | 7,0872 (0,10×) |
| `guided` | não | 0,1796 (3,9×) | 0,1461 (4,8×) | 0,1397 (5,0×) |
| `guided` | `collapse(2)` | 0,1925 (3,6×) | 0,1502 (4,7×) | 0,1476 (4,7×) |

## Análise

**`static` vence em todas as configurações.** Toda célula custa o mesmo: cinco
leituras, uma multiplicação e uma soma por componente. Não há desbalanceamento
para corrigir, então não há nada que justifique pagar por distribuição dinâmica.
`static` divide as linhas antes de começar e depois nenhuma thread consulta nada
compartilhado.

**`dynamic` paga o contador do laço.** Cada bloco entregue exige um acesso
sincronizado. O revelador é que `dynamic` não melhora com mais threads: 0,2290 s
com 16 e 0,2866 s com 64, ou seja, termina pior do que começou. Mais threads
disputam o mesmo contador, e o trabalho por bloco é pequeno demais para esconder
o custo. `guided` fica sempre à frente do `dynamic`, porque começa com blocos
grandes e faz muito menos entregas, mas empaca em torno de 5× e continua
consultando um contador que `static` não consulta.

Repetindo o experimento inteiro, os tempos de `static` se reproduziram dentro de
1%, enquanto `dynamic` e `guided` variaram bem mais em 64 threads — `guided`
chegou a 0,2345 s numa repetição contra 0,1397 s nesta. Essa instabilidade é ela
própria um resultado: o custo dessas políticas depende de disputa em tempo de
execução, que muda de uma execução para outra. O de `static` não depende.

**`dynamic` com `collapse(2)` é o pior caso.** O espaço passa a 1.048.576
iterações com bloco 1: cerca de 210 milhões de entregas sincronizadas na
simulação, cada uma para calcular **uma célula**. Dá 7 s contra 0,022 s da melhor
versão — 10 vezes mais lento que o código sequencial. É a demonstração mais
direta de que uma cláusula pode custar muito mais do que o cálculo que distribui.

**`collapse(2)` só atrapalha aqui.** Ele serve quando o laço externo tem
iterações de menos para ocupar as threads. Não é o caso: 1.024 linhas para no
máximo 64 threads, 16 linhas por thread. O que ele acrescenta é custo de
indexação — recuperar `i` e `j` de um índice linear a cada iteração, e perder o
laço interno simples que o compilador otimiza bem. Com `static`, custou de 1,5×
a 1,9× de tempo a mais.

**O speedup é sublinear, e isso é esperado.** Com `static`, a eficiência cai de
77% (16 threads) para 69% (32) e 50% (64). O problema é limitado por memória: a
cada passo o programa percorre quatro matrizes de 8 MB, muito além do cache, com
poucas operações por byte lido. Estimando pela via compulsória — 32 MB por passo,
200 passos, 6,4 GB em 0,022 s — dá cerca de 290 GB/s, compatível com o limite de
um nó de dois sockets. Some-se a isso um gargalo que **não foi corrigido de
propósito**: as matrizes são inicializadas sequencialmente, então por *first
touch* quase toda a memória fica perto de um socket só e as threads do outro
pagam acesso remoto. Inicializar em paralelo distribuiria as páginas, mas
otimizar o laço mediria a otimização, não as cláusulas, que é o que o enunciado
manda explorar.

## Código

{{CODIGO:difusao_viscosa.c}}
