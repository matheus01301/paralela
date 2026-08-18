# Tarefa 1 — Aproximação de π por séries, com análise de acurácia e paralelismo

Programa que calcula aproximações de π por série matemática, variando o número de
iterações, medindo o tempo de execução e analisando como a acurácia evolui com mais
processamento. Cada exercício foi implementado **em C e em TypeScript**, com a mesma
estrutura, para comparar as duas plataformas sob o mesmo protocolo de medição.

## Ambiente das medições

| | |
|---|---|
| CPU | Intel i7-13650HX — 14 núcleos (6 P-cores + 8 E-cores), 20 threads |
| SO | Windows 11 |
| Compilador C | gcc 16.1.0 (WinLibs UCRT), OpenMP 5.2 |
| Runtime JS | Node v22.19.0 (V8 12.4) |

## Protocolo de medição

Fixo para todos os programas, para que os números sejam comparáveis entre si:

- **Tempo de parede** para calcular speedup (`QueryPerformanceCounter` no C,
  `performance.now()` no Node — nunca `Date.now()`, que tem resolução de ~1 ms e
  pode andar para trás com ajuste de NTP). O tempo de CPU aparece numa coluna
  separada, para mostrar as duas métricas descolando quando há paralelismo.
- **Aquecimento de JIT obrigatório** nas versões TypeScript: sem ele, as primeiras
  medições mediriam o interpretador do V8 e não o código otimizado.
- **Mediana de várias execuções**, nunca média. O Node varia ~7,6% entre execuções
  (GC + recompilação do JIT) contra ~1,2% do C, então a versão web precisa de mais
  repetições.
- **`-O2` sempre declarado.** Com `-O0` o mesmo código roda 2–5× mais devagar e os
  números não são comparáveis.

## Arquivos

| arquivo | o que faz |
|---|---|
| `pi_serie.c` / `pi_serie.ts` | **O exercício principal.** Séries de Leibniz e Nilakantha, N de 10¹ a 10⁹, com erro, casas decimais corretas e tempo |
| `pi_omp.c` | Versão paralela com OpenMP: condição de corrida, custo de seção crítica, speedup e métrica de Karp-Flatt |
| `pi_workers.mts` | Equivalente em Node com `worker_threads`, com coluna isolando o overhead |
| `sched_test.c` | Compara `schedule(static)`, `dynamic` e `guided` |
| `memoria_demo.mts` | Demonstra condição de corrida em JS com `SharedArrayBuffer` e a correção com `Atomics` |
| `overhead_test.c` / `.mts` | Decompõe o custo de criar uma thread e um worker, e o custo de mover dados |

## Como compilar e executar

```bash
# Série serial
gcc -O2 -Wall -Wextra -std=c99 pi_serie.c -o pi_serie.exe -lm
./pi_serie.exe 9
node --experimental-strip-types pi_serie.ts 9

# Paralelo
gcc -O2 -Wall -Wextra -fopenmp pi_omp.c -o pi_omp.exe -lm
./pi_omp.exe
node --experimental-strip-types pi_workers.mts

# Escalonamento e overhead
gcc -O2 -Wall -Wextra -fopenmp sched_test.c -o sched_test.exe -lm
gcc -O2 -Wall -Wextra -fopenmp overhead_test.c -o overhead_test.exe
node --experimental-strip-types memoria_demo.mts
node --experimental-strip-types overhead_test.mts
```

---

## Resultados

### 1. Acurácia × iterações

As duas séries custam O(N) — o mesmo tempo por iteração. A diferença está na
qualidade de cada iteração:

| série | erro após N termos | o que 10× mais trabalho compra |
|---|---|---|
| Leibniz-Gregory | ≈ 1/N | ~1 casa decimal |
| Nilakantha | ≈ 1/(4N³) | ~3 casas decimais |

Medido: o erro de Leibniz caiu por **exatamente 10,0** em todos os oito passos de N;
o de Nilakantha por **~970** (≈10³) até saturar.

**Retorno decrescente:** Leibniz precisaria de ~10¹⁵ iterações para as 15 casas que
um `double` comporta. Paralelismo não resolve isso — 8 núcleos dividem o tempo por
8, mas seria preciso dividir por 10⁶. Paralelismo dá um fator constante; a ordem de
convergência dá ordens de grandeza.

**Existe um teto absoluto.** De N=10⁶ a N=10⁹, o erro do Nilakantha travou em
`6.217e-15` sem mudar **um único dígito**. O motivo: o termo k vale ≈ 0,5/k³, e o
*ulp* de um `double` perto de 3,14 é 4,44e-16. Quando o termo fica menor que meio
ulp — em k ≈ 1,3·10⁵ — a soma não altera nenhum bit. **999 milhões de iterações
adicionais, 1,4 s de CPU, e o resultado é bit a bit o mesmo.** É o argumento
empírico contra "é só jogar mais processamento no problema".

### 2. C × TypeScript (serial, N=10⁹, mediana de 3)

| | C (gcc -O2) | TypeScript (Node 22) | C é |
|---|---:|---:|---:|
| Leibniz | 0,844 s | 1,069 s | 1,27× |
| Nilakantha | 0,942 s | 1,489 s | 1,58× |
| variação entre execuções | ±1,2% | ±7,6% | |

**Os valores saíram idênticos dígito a dígito nas 18 linhas da tabela.** O `number`
do JavaScript é um `double` IEEE-754 de 64 bits, o mesmo tipo do C — mesma mantissa
de 53 bits, mesmo arredondamento. **A linguagem não afeta a acurácia, só o tempo.**

Por que a diferença é tão pequena: 1 bilhão de iterações em 0,844 s são ~3,8 ciclos
por iteração. A vazão da unidade divisora de ponto flutuante desse processador é de
~4 ciclos. **O C está no limite do hardware** — não há para onde otimizar, e por isso
o TypeScript chega perto. A diferença é maior no Nilakantha (1,58×) porque o laço
dele tem três multiplicações além da divisão, sobrando trabalho fora do divisor para
o otimizador do GCC explorar.

### 3. Paralelismo: OpenMP × worker_threads

| | C / OpenMP | TypeScript / workers |
|---|---:|---:|
| melhor tempo | **0,117 s** (19 threads) | **0,226 s** (13 workers) |
| melhor speedup | **7,50×** | **4,11×** |
| tempo do *laço numérico* com 20 threads | 0,122 s | **0,126 s** |
| overhead com 20 threads | ~0 | **0,134 s (52% do total)** |

O achado principal está na penúltima linha: **o cálculo em si paralelizou
praticamente igual nas duas plataformas** (0,122 s contra 0,126 s). O laço do Node
escalou 6,8×, quase encostando nos 7,2× do OpenMP. Mas o tempo *total* só escalou
3,6×, porque metade do tempo de parede virou logística de isolates.

Trocando `schedule(static)` por `dynamic`, o melhor speedup do OpenMP subiu de 7,50×
para **8,28×** — uma palavra na diretiva.

### 4. Condição de corrida

Sem a cláusula `reduction`, com 20 threads, três execuções seguidas:

```
pi = 0.000000105263158
pi = 0.000000022222222
pi = 0.000000007352941
```

π foi destruído, e com valor diferente a cada vez. **Em C basta esquecer uma
cláusula** — compila limpo, roda sem aviso. Em Node, reproduzir o mesmo bug exigiu
montar um `SharedArrayBuffer` de propósito (`memoria_demo.mts`), porque cada worker
é um isolate com heap próprio e não há nada compartilhado para disputar.

### 5. Custo da correção

| | tempo | |
|---|---:|---|
| `omp critical` | 0,0670 s | correto, serializa |
| `reduction` | 0,0010 s | correto, privatiza |

**67× de diferença entre duas versões igualmente corretas.** Correção via
serialização é cara; via privatização é quase de graça.

### 6. Por que a eficiência cai (`sched_test.c`)

| threads | static | dynamic | guided |
|---:|---:|---:|---:|
| 6 | 0,2190 s | **0,1630 s** | 0,1690 s |
| 14 | 0,1350 s | 0,1260 s | 0,1210 s |
| 20 | 0,1120 s | 0,1060 s | 0,1060 s |

Duas causas, em regimes diferentes:

- **Com 6 threads, `dynamic` ganhou 34%** — é desbalanceamento. O processador é
  heterogêneo (6 P-cores rápidos, 8 E-cores lentos) e o `static` corta em fatias
  iguais, deixando o P-core esperando o E-core.
- **Com 20 threads, `dynamic` ganhou só 6%** — o limite passa a ser físico: 20
  threads lógicas em 14 núcleos, e as irmãs de hyperthreading **compartilham a
  unidade divisora**. Como este laço é limitado pelo divisor, nenhum escalonamento
  resolve.

### 7. Custo de criar uma linha de execução

| | por unidade | |
|---|---:|---:|
| worker do Node (novo) | **21.630 µs** | 1× |
| thread OpenMP (criação) | ~125 µs | ~170× mais barato |
| thread OpenMP (pool acordado) | **3,8 µs** | ~5.700× mais barato |

Decomposição do worker: 21,63 ms para o isolate + bootstrap do Node (84%), +2,04 ms
para compilar o módulo, +2,11 ms para aquecer o JIT. **Nenhuma dessas etapas move um
byte de dado do usuário.**

A thread é barata porque não faz quase nada: não cria heap, não cria GC, não carrega
runtime, não compila código. Uma thread é um contador de programa e uma pilha; um
worker é praticamente um processo.

*(Ressalva: `omp_get_wtime()` tem granularidade de ~1 ms aqui, então os ~125 µs por
thread têm incerteza grande. O número do pool, medido sobre 10.000 repetições, é
confiável.)*

### 8. Custo de mover dados entre isolates

| tamanho | cópia | Transferable | SharedArrayBuffer |
|---:|---:|---:|---:|
| 1 MB | 0,95 ms | 0,02 ms | 0,03 ms |
| 100 MB | 72,13 ms | 0,13 ms | 0,08 ms |
| 500 MB | **363,86 ms** | **0,11 ms** | **0,07 ms** |

A cópia é linear (~0,73 ms/MB ≈ 1,4 GB/s); `Transferable` e `SharedArrayBuffer` são
**constantes**, porque nada é copiado. **500 MB: 3.300× de diferença.**

Consequência prática — a granularidade mínima que compensa paralelizar:

| plataforma | custo de acionar | trabalho mínimo |
|---|---:|---:|
| OpenMP (pool) | 76 µs/região | ~1 ms |
| Node, worker novo a cada vez | 21,6 ms | ~200 ms |
| Node, pool de workers | ~0,1 ms | ~1 ms |

---

## Conclusões

1. **A acurácia não depende da linguagem.** `number` e `double` são o mesmo
   IEEE-754; os resultados saíram bit a bit idênticos. O que muda é o tempo.
2. **Mais processamento tem retorno decrescente, e depois retorno zero.** Trocar o
   algoritmo (Leibniz → Nilakantha) valeu mais que qualquer paralelização, e a partir
   do limite de precisão do `double` nenhuma iteração adicional melhora nada.
3. **O paralelismo dá um fator constante limitado pelo hardware.** 7,5× em 20
   threads (eficiência de 36%), limitado por núcleos heterogêneos e pelo
   compartilhamento da unidade divisora entre irmãs de hyperthreading.
4. **Node não é mais lento; é mais caro de paralelizar.** O laço numérico escalou
   quase igual ao do OpenMP. A diferença é inteiramente o modelo de memória: cada
   thread exige um isolate de 21,6 ms, porque o heap e o GC do V8 não são
   thread-safe.
5. **Correção em concorrência se paga com serialização** — 67× entre `critical` e
   `reduction`, ambas corretas.
