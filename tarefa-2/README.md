# Tarefa 2 — Multiplicação matriz-vetor: acesso por linhas × por colunas

Duas versões de `y = A·x` em C que fazem **exatamente as mesmas 2·N² operações de
ponto flutuante sobre exatamente os mesmos dados**, trocando apenas a ordem dos
laços. Toda diferença de tempo vem, portanto, do padrão de acesso à memória.

## Ambiente

| | |
|---|---|
| CPU | Intel i7-13650HX (Raptor Lake), 14 núcleos / 20 threads |
| L1d (P-core) | 48 KB → matriz cabe até N ≈ 78 |
| L2 (P-core) | 1,25 MB → matriz cabe até N ≈ 404 |
| L3 (compartilhado) | 24 MB → matriz cabe até N ≈ **1774** |
| Linha de cache | 64 bytes = **8 doubles** |
| Página de memória | 4 KB = 512 doubles |
| Compilador | gcc 16.1.0, `-O2 -Wall -Wextra -std=c99` |
| Relógio | `clock_gettime(CLOCK_MONOTONIC)` |

## As duas versões

Em C a matriz é armazenada **por linhas** (*row-major*): `A[i][j]` e `A[i][j+1]` são
vizinhos na memória; `A[i][j]` e `A[i+1][j]` estão separados por `N·8` bytes.

```c
/* POR LINHAS - laço interno varia j: percorre a memória sequencialmente */
for (i = 0; i < n; i++) {
    double soma = 0.0;
    for (j = 0; j < n; j++) soma += A[i*n + j] * x[j];
    y[i] = soma;
}

/* POR COLUNAS - laço interno varia i: salta de N*8 em N*8 bytes */
for (j = 0; j < n; j++) {
    double xj = x[j];
    for (i = 0; i < n; i++) y[i] += A[i*n + j] * xj;
}
```

As duas somam sobre `j` na mesma ordem, então os resultados devem bater **bit a
bit** — o programa verifica isso com `memcmp` e acusaria qualquer divergência. Em
todas as execuções os resultados foram idênticos.

## Metodologia

- `clock_gettime(CLOCK_MONOTONIC)`: relógio monotônico com resolução de
  nanossegundos. Não usamos `CLOCK_REALTIME` (pode saltar com ajuste de NTP e gerar
  intervalos negativos) nem `clock()` (mede tempo de CPU, não de parede).
- **Aquecimento antes de medir**, para que o *first touch* das páginas não seja
  contabilizado como falha de cache.
- Número de repetições ajustado por tamanho, para cada medição durar o suficiente;
  **mediana** de 3 a 5 amostras.
- Dados determinísticos, sem NaN nem denormais, para o tempo depender do acesso e
  não do valor dos números.

## Como executar

```bash
gcc -O2 -Wall -Wextra -std=c99 mxv.c -o mxv.exe -lm

./mxv.exe             # varredura em potências de 2, N = 64 .. 8192
./mxv.exe fino        # passos finos em volta das fronteiras de L2 e L3
./mxv.exe conflito    # isola o efeito de conflito de conjunto (padding)
```

---

## Resultado 1 — Varredura geral

| N | matriz | cabe em | linhas (s) | colunas (s) | razão |
|---:|---:|:---:|---:|---:|---:|
| 64 | 0,03 MB | L1 | 0,000001 | 0,000001 | 1,53× |
| 128 | 0,13 MB | L2 | 0,000005 | 0,000008 | 1,57× |
| 256 | 0,50 MB | L2 | 0,000021 | 0,000031 | 1,47× |
| 512 | 2,0 MB | L3 | 0,000098 | 0,000545 | **5,54×** |
| 1024 | 8,0 MB | L3 | 0,000440 | 0,002251 | 5,12× |
| 2048 | 32 MB | RAM | 0,002240 | 0,044399 | **19,82×** |
| 4096 | 128 MB | RAM | 0,010048 | 0,212255 | 21,12× |
| 8192 | 512 MB | RAM | 0,036190 | 0,874614 | **24,17×** |

A versão por linhas manteve 3.300–8.900 MFLOP/s em toda a faixa. A por colunas caiu
de 5.900 para 153 MFLOP/s — **38× de degradação sem que uma única operação
aritmética mudasse**.

## Resultado 2 — Varredura fina: a divergência não é monotônica

| N | razão | | N | razão |
|---:|---:|---|---:|---:|
| 384 | 1,35× | | 1280 | 6,45× |
| 448 | 1,67× | | 1536 | 7,64× |
| 480 | 1,82× | | 1792 | 8,59× |
| **512** | **5,30×** | | 1920 | 7,19× |
| 576 | 1,72× | | **2048** | **19,58×** |
| 640 | 1,77× | | 2304 | 9,33× |

Aqui apareceu algo que a varredura em potências de 2 escondia: **N = 512 é 3× pior
que N = 576, e N = 2048 é 2× pior que N = 2304 — apesar de serem matrizes
menores.** Isso não pode ser capacidade de cache: uma matriz menor não pode causar
mais falhas por falta de espaço. É um efeito diferente.

## Resultado 3 — A prova: conflito de conjunto

Mesmo N, mudando apenas a distância entre linhas consecutivas de `N` para `N+1`
elementos (um único `double` de preenchimento por linha):

| N | colunas, ld=N | colunas, ld=N+1 | ganho | linhas, ld=N | linhas, ld=N+1 | ganho |
|---:|---:|---:|---:|---:|---:|---:|
| 512 | 0,000527s | 0,000087s | **6,03×** | 0,000099s | 0,000098s | 1,02× |
| 1024 | 0,002806s | 0,000505s | **5,55×** | 0,000444s | 0,000423s | 1,05× |
| 2048 | 0,045662s | 0,007094s | **6,44×** | 0,002185s | 0,002168s | 1,01× |
| 4096 | 0,206473s | 0,086489s | **2,39×** | 0,008996s | 0,009250s | 0,97× |

**Oito bytes a mais por linha aceleraram a versão por colunas em até 6,44×, e não
mexeram em nada na versão por linhas.** Está provado que existem duas causas
independentes.

---

## Análise: a partir de que tamanho os tempos divergem, e por quê

### Causa 1 — Aproveitamento da linha de cache (o efeito de fundo)

A memória não é transferida byte a byte, e sim em **linhas de cache de 64 bytes = 8
doubles**. Esse é o ponto de partida de tudo.

- **Por linhas:** os 8 doubles trazidos são consumidos nas 8 iterações seguintes.
  Aproveitamento de 100%, e o *prefetcher* do processador reconhece o padrão
  sequencial e busca as próximas linhas antes de serem pedidas.
- **Por colunas:** cada acesso salta `N·8` bytes e cai numa linha diferente. Dos 8
  doubles trazidos, **1 é usado**. Os outros 7 só interessam quando o laço chegar às
  colunas `j+1 … j+7` — o que só acontece depois de percorrer a coluna inteira.

Daí a previsão teórica: **se a matriz não couber no cache, a versão por colunas lê
a memória 8 vezes**, porque cada linha de cache é buscada uma vez por coluna.

### Causa 2 — Capacidade: a partir de N ≈ 1774

Enquanto a matriz cabe no cache, o desperdício acima **não custa nada**: as linhas
com os 7 doubles ainda não usados continuam residentes, e quando a coluna seguinte
precisar deles, estarão lá. A partir do momento em que a matriz não cabe, elas já
terão sido despejadas e a busca vai à memória de novo.

A fronteira é o L3 de 24 MB: `N² · 8 > 24 MB` ⟹ **N > 1774**.

Isolando esse efeito (com o padding que elimina a Causa 3), a razão pura de
capacidade é:

| N | matriz | razão sem conflito |
|---:|---:|---:|
| 512 | 2 MB | 0,89× |
| 1024 | 8 MB | 1,19× |
| 2048 | 32 MB | **3,27×** |
| 4096 | 128 MB | **9,35×** |

**A divergência por capacidade começa exatamente onde a teoria previa: entre N=1024
(8 MB, cabe no L3, razão 1,19×) e N=2048 (32 MB, não cabe, razão 3,27×).** Em
N=4096 ela atinge **9,35×**, muito perto do fator 8 previsto pelo aproveitamento da
linha de cache — o excedente vem da perda do *prefetcher*, que não consegue prever
acesso com passo grande.

Vale notar o caso de N=512 com padding: razão **0,89×**, ou seja, a versão por
colunas ficou *mais rápida*. Faz sentido — quando a memória não é o gargalo, a
versão por colunas tem mais paralelismo em nível de instrução: os `y[i] += …` de
diferentes `i` são independentes entre si, enquanto o `soma += …` da versão por
linhas é uma cadeia de dependências (cada soma espera a anterior, ~4 ciclos de
latência).

### Causa 3 — Conflito de conjunto: sempre que N é potência de 2

Um cache não é totalmente associativo. Ele é dividido em **conjuntos**, e o conjunto
de um endereço é escolhido por alguns bits do meio do endereço. Cada conjunto guarda
apenas *W* linhas (W = associatividade, tipicamente 8 a 16).

Quando `N` é potência de 2, o passo `N·8` bytes também é potência de 2 — ou seja,
**altera apenas bits altos do endereço e deixa os bits de índice de conjunto
intactos**. Resultado: todos os elementos de uma mesma coluna caem no **mesmo
conjunto**. Depois de W elementos, eles começam a se despejar mutuamente, e o cache
inteiro fica inutilizado para aquela coluna, por mais espaço livre que haja.

Isso explica por que N=512 (5,30×) é pior que N=576 (1,72×) mesmo sendo uma matriz
menor: 512 é potência de 2, 576 não é.

O padding de um elemento por linha resolve porque desloca cada linha em 8 bytes, de
modo que os elementos de uma coluna passam a cair em conjuntos diferentes. **Ganho
medido de até 6,44×, com uma linha de código.**

### Causa 4 — TLB, coincidindo em N = 512

A tradução de endereço virtual para físico é acelerada pela **TLB**, que guarda umas
poucas dezenas de traduções. Com páginas de 4 KB = 512 doubles, o passo da versão
por colunas cruza uma página inteira quando **N ≥ 512**. A partir daí, *cada* acesso
do laço interno cai numa página diferente, e a TLB — que não tem entradas suficientes
para uma coluna inteira — falha em praticamente todos.

É por isso que N=512 é um ponto especialmente ruim: as Causas 3 e 4 batem no mesmo
tamanho.

---

## Conclusões

1. **A divergência significativa por capacidade começa entre N=1024 e N=2048**, que
   é exatamente onde a matriz (8 MB → 32 MB) ultrapassa o L3 de 24 MB. A razão vai
   de 1,19× para 3,27×, e chega a 9,35× em N=4096 — próximo do fator 8 previsto por
   "1 de cada 8 doubles da linha de cache é aproveitado".

2. **Sobreposto a isso há um efeito de conflito de conjunto que aparece em qualquer
   tamanho potência de 2**, inclusive quando a matriz cabe folgadamente no cache. Ele
   é responsável pelos picos de 5,54× em N=512 e 19,82× em N=2048, e some com um
   `double` de preenchimento por linha.

3. **A mesma quantidade de trabalho aritmético pode custar 24× mais tempo.** As duas
   versões fazem 2·N² operações idênticas sobre dados idênticos e produzem resultados
   bit a bit iguais. A diferença é inteiramente o padrão de acesso.

4. **A ordem dos laços deve seguir a ordem de armazenamento.** Em C (row-major), o
   índice que varia mais rápido precisa ser o último — o da coluna. Em Fortran
   (column-major) a regra é a oposta. Essa é a razão pela qual bibliotecas de álgebra
   linear expõem o parâmetro `ld` (*leading dimension*): ele existe justamente para
   permitir o padding que elimina o conflito de conjunto.

5. **Cuidado ao escolher os tamanhos de teste.** Uma varredura só em potências de 2 —
   o instinto natural — mistura os efeitos de capacidade e de conflito, e leva a
   superestimar o primeiro. Foi a varredura fina que separou os dois.
