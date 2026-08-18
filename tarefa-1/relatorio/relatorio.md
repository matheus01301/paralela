# Aproximação de π por séries: acurácia, tempo e o custo do último dígito

## 1. O que foi feito

- **Duas séries** que convergem para π, implementadas em C: **Leibniz-Gregory**
  (erro ∝ 1/N) e **Nilakantha** (erro ∝ 1/4N³). As duas têm **custo O(N) idêntico**,
  então a comparação isola uma única variável: a **ordem de convergência**.
- **N de 10¹ a 10⁹**, medindo tempo de parede e de CPU, erro absoluto contra uma
  referência de 21 dígitos e o número de casas decimais corretas.
- **Mediana de 3 execuções.** Compilado com `gcc -O2 -Wall -Wextra -std=c99`, sem
  emitir nenhum aviso.
- **Achado principal:** existe um ponto em que processamento adicional **não compra
  nada** — e o limite é da **representação em ponto flutuante**, não do hardware.

## 2. As duas séries

<div class="formula">Leibniz-Gregory: &nbsp; π = 4 · ( 1 − 1/3 + 1/5 − 1/7 + ⋯ ) &nbsp;&nbsp;·&nbsp;&nbsp; termo k = (−1)<sup>k</sup>/(2k+1) &nbsp;·&nbsp; erro ≈ 1/N</div>

<div class="formula">Nilakantha: &nbsp; π = 3 + 4/(2·3·4) − 4/(4·5·6) + ⋯ &nbsp;&nbsp;·&nbsp;&nbsp; termo k = (−1)<sup>k+1</sup>·4/[a(a+1)(a+2)], a = 2k &nbsp;·&nbsp; erro ≈ 1/4N³</div>

Mesmo número de operações por termo, dentro de um fator pequeno. **A diferença não
está no custo da iteração, está na qualidade da iteração.**

## 3. Metodologia e decisões de implementação

- **Tempo de parede** (`QueryPerformanceCounter`, nanossegundos, monotônico), porque é
  a única grandeza com que se calcula *speedup*. O **tempo de CPU** (`clock()`) fica em
  coluna separada: em código serial coincidem, no paralelo descolam.
- **Mediana, não média** — a média é deslocada por uma execução contaminada por outro
  processo.
- **`-O2` sempre declarado.** Com `-O0` o mesmo código roda várias vezes mais devagar e
  os números não são comparáveis.
- **Referência de π com 21 dígitos**, não `M_PI`: `M_PI` não é padrão C, e a referência
  precisa de mais dígitos do que o `double` guarda, senão o erro medido seria em parte
  o erro da referência.
- **Sem `pow(-1.0, k)`** para alternar o sinal: seria uma chamada de função de dezenas
  de ciclos executada um bilhão de vezes. Multiplicar por −1 custa 1 ciclo. Também
  **sem `pow(10, e)`** para gerar N — devolve `double` e a conversão pode dar
  999999999.
- **Cronômetro só em volta do laço numérico**, com o `printf` fora: em N = 10 a E/S
  dominaria a medição.

**Ambiente:** Intel i7-13650HX (14 núcleos: 6 P-cores + 8 E-cores, 20 threads),
Windows 11, gcc 16.1.0 (MinGW-w64 UCRT).

## 4. Resultados

Mediana de 3 execuções. As aproximações e os erros saíram **idênticos dígito a dígito
nas três** — o cálculo é determinístico, só o tempo varia. Referência:
π = 3,14159265358979323846.

<table class="resultados">
<thead>
<tr><th rowspan="2">N</th><th colspan="4">Leibniz-Gregory</th><th colspan="4">Nilakantha</th></tr>
<tr><th>aproximação</th><th>erro</th><th>casas</th><th>t (s)</th><th>aproximação</th><th>erro</th><th>casas</th><th>t (s)</th></tr>
</thead>
<tbody>
<tr><td>10</td><td>3,041839618929403</td><td>9,975e−02</td><td>1,00</td><td>—</td><td>3,141406718496502</td><td>1,859e−04</td><td>3,73</td><td>—</td></tr>
<tr><td>10²</td><td>3,131592903558554</td><td>1,000e−02</td><td>2,00</td><td>—</td><td>3,141592410971982</td><td>2,426e−07</td><td>6,62</td><td>—</td></tr>
<tr><td>10³</td><td>3,140592653839794</td><td>1,000e−03</td><td>3,00</td><td>—</td><td>3,141592653340544</td><td>2,492e−10</td><td>9,60</td><td>—</td></tr>
<tr><td>10⁴</td><td>3,141492653590034</td><td>1,000e−04</td><td>4,00</td><td>—</td><td>3,141592653589538</td><td>2,549e−13</td><td>12,59</td><td>—</td></tr>
<tr><td>10⁵</td><td>3,141582653589720</td><td>1,000e−05</td><td>5,00</td><td>0,0001</td><td>3,141592653589786</td><td>6,661e−15</td><td>14,18</td><td>0,0001</td></tr>
<tr><td>10⁶</td><td>3,141591653589774</td><td>1,000e−06</td><td>6,00</td><td>0,0009</td><td>3,141592653589787</td><td><b>6,217e−15</b></td><td>14,21</td><td>0,0010</td></tr>
<tr><td>10⁷</td><td>3,141592553589792</td><td>1,000e−07</td><td>7,00</td><td>0,0088</td><td>3,141592653589787</td><td><b>6,217e−15</b></td><td>14,21</td><td>0,0099</td></tr>
<tr><td>10⁸</td><td>3,141592643589326</td><td>1,000e−08</td><td>8,00</td><td>0,0884</td><td>3,141592653589787</td><td><b>6,217e−15</b></td><td>14,21</td><td>0,0982</td></tr>
<tr><td>10⁹</td><td>3,141592652588050</td><td>1,002e−09</td><td>9,00</td><td>0,8723</td><td>3,141592653589787</td><td><b>6,217e−15</b></td><td>14,21</td><td>0,9534</td></tr>
</tbody>
</table>

*Tempos abaixo de 10⁵ ficam sob a granularidade útil do cronômetro.*

### 4.1 Fator de melhoria a cada 10× em N

| N | erro ÷ Leibniz | erro ÷ Nilakantha | tempo × | | N | erro ÷ Leibniz | erro ÷ Nilakantha | tempo × |
|---:|---:|---:|---:|---|---:|---:|---:|---:|
| 10² | 10,0 | 766,4 | — | | 10⁶ | 10,0 | 1,1 | 10,0 |
| 10³ | 10,0 | 973,4 | — | | 10⁷ | 10,0 | 1,0 | 10,1 |
| 10⁴ | 10,0 | 977,8 | 9,2 | | 10⁸ | 10,0 | 1,0 | 10,1 |
| 10⁵ | 10,0 | 38,3 | 9,9 | | 10⁹ | 10,0 | 1,0 | 10,2 |

**Três leituras diretas:**

1. **Custo rigorosamente linear** — 10× iterações, 10,0–10,2× tempo, nas duas séries.
   Confirma O(N) e valida a medição.
2. **A ordem de convergência aparece nos dígitos** — Leibniz divide o erro por
   **exatamente 10,0** nos oito passos; Nilakantha por **~970 ≈ 10³**.
3. **Nilakantha para de melhorar em N = 10⁶** — o fator cai a 1,0. Não é ruído: é um
   teto, e ele é absoluto (Seção 5.3).

## 5. Análise

### 5.1 Retorno decrescente desde o primeiro passo

Cada casa decimal de Leibniz custa **10× a anterior**: progressão geométrica no custo
para progressão aritmética no resultado. Extrapolando de N = 10⁹ (8,7 × 10⁻¹⁰ s por
iteração):

| casas decimais | 9 (medido) | 12 | 14 | 15 |
|---|---|---|---|---|
| N necessário | 10⁹ | 10¹² | 10¹⁴ | 10¹⁵ |
| tempo, 1 núcleo | 0,87 s | ~14,5 min | **~24 h** | ~10 dias |

E 15 casas é o limite do que um `double` comporta — não há N que vá além.

### 5.2 Trocar o algoritmo × paralelizar

Para **14 casas decimais corretas**:

| caminho | custo | ganho |
|---|---|---|
| Leibniz, 1 núcleo | ~24 h (N = 10¹⁴, extrapolado) | — |
| Leibniz, 20 threads (`reduction`) | ~3,6 h | **6,71×** medido |
| **Nilakantha, 1 núcleo** | **0,0001 s** (N = 10⁵, medido) | **~10⁸** |

- *Speedup* de **6,71×** com 18 threads, **eficiência de 37%** — medido em `pi_omp.c`,
  no mesmo N = 10⁹ e na mesma máquina.
- A eficiência cai por duas causas: processador **heterogêneo** (P-cores rápidos e
  E-cores lentos desbalanceiam fatias iguais) e threads irmãs de **hyperthreading
  compartilhando a unidade divisora**, que é o recurso que limita este laço.
- **Karp-Flatt** estimou fração serial de **0,08 a 0,13, crescendo com *p*** —
  assinatura de overhead de paralelização, não de código serial. Amdahl com essa fração
  dá teto de 8× a 12×.
- **Paralelismo não corrige ordem de convergência ruim.** Vinte núcleos dividem o tempo
  por 6,71; seria preciso dividir por 10⁸.

### 5.3 O teto absoluto: 999 milhões de iterações que não mudam um bit

- De N = 10⁶ a 10⁹ o erro do Nilakantha ficou em **6,217 × 10⁻¹⁵** e a aproximação em
  **3,141592653589787**, sem alterar **um único dígito** em quatro ordens de grandeza.
- Causa: o termo *k* vale ≈ 4/(8*k*³) = 0,5/*k*³. O ***ulp*** do `double` perto de 3,14
  é 2⁻⁵¹ ≈ 4,44 × 10⁻¹⁶. A soma só altera algum bit se o termo passar de meio *ulp*:

<div class="formula">0,5 / k³ &gt; 2,22 × 10⁻¹⁶ &nbsp;&nbsp;⟹&nbsp;&nbsp; k &lt; 1,3 × 10⁵</div>

- A partir de *k* ≈ 130 000 cada termo é **absorvido** — arredondado de volta ao mesmo
  valor. O laço executa, gasta ~0,95 s de CPU, e o acumulador é **idêntico bit a bit**.
- É o argumento empírico contra "é só jogar mais processamento no problema": o retorno
  não é apenas decrescente, é **exatamente zero**. E o ponto é definido pela
  representação, não pelo hardware.

### 5.4 Efeito colateral: paralelizar muda os últimos dígitos

O erro de Leibniz em N = 10⁹ passa de 1,002 × 10⁻⁹ com 1 thread para 9,995 × 10⁻¹⁰ com
20: a soma de ponto flutuante **não é associativa**, e o número de threads muda o
agrupamento dos termos. **A versão paralela não reproduz a serial bit a bit, e as duas
estão corretas.** Onde há exigência de reprodutibilidade exata, isso obriga a
somatórios de ordem fixa.

## 6. O mesmo comportamento em aplicações reais

**Simulações físicas**

- **Diferenças finitas de 2ª ordem, 3D:** erro O(*h*²). Uma casa decimal exige refinar
  *h* por √10 → 32× mais células e, pela **condição CFL** (Δ*t* ∝ *h*), 3,16× mais
  passos: **~100× por casa decimal**, pior que Leibniz. Daí o investimento em **métodos
  de ordem alta** (espectrais, Runge-Kutta 4, malha adaptativa) — a mesma troca de
  Leibniz por Nilakantha.
- **Monte Carlo:** erro ∝ 1/√N, **100× mais amostras por casa decimal**. Justifica
  **redução de variância**, amostragem por importância e sequências quasi-aleatórias.
- **O piso também existe:** somas de bilhões de contribuições acumulam arredondamento,
  e a resposta é **somatório de Kahan** ou precisão mista, não mais iterações. Em
  sistemas caóticos o **expoente de Lyapunov** impõe horizonte de previsibilidade de
  ~duas semanas na previsão do tempo, **independentemente do poder computacional**.
- **Tolerância de engenharia:** 1% num coeficiente de arrasto é atingido muito antes do
  limite numérico. Processamento além dela é desperdício.

**Inteligência artificial**

- **Leis de escala:** a perda decai como *L* ∝ *C*⁻ᵅ com α ≈ 0,05 nos trabalhos
  publicados. Com esse expoente, **reduzir a perda pela metade exige ~2²⁰ ≈ 10⁶ vezes
  mais compute** — mesmo formato da Seção 5.1, com expoente pior que o de Leibniz.
- **Existe um piso:** a perda converge para um resíduo irredutível, dado pela entropia
  dos dados. Nenhum compute o atravessa, como nenhuma iteração atravessa o *ulp*.
- **A saída foi trocar o algoritmo:** atenção em vez de recorrência, mistura de
  especialistas, destilação, quantização. Cada uma é uma "Nilakantha" — muda o expoente
  da curva em vez de andar mais longe sobre uma curva ruim.
- **Inversão instrutiva sobre precisão:** aqui queríamos as 15 casas do `double`; no
  treinamento o caminho foi `float32` → `bfloat16` → `fp8`, jogando casas fora de
  propósito. **A precisão necessária é propriedade do problema, não virtude em si.**

| domínio | erro × trabalho | custo de +1 dígito | piso |
|---|---|---:|---|
| Leibniz | 1/N | 10× | precisão do `double` |
| Nilakantha | 1/(4N³) | 2,2× | precisão do `double` |
| Monte Carlo | 1/√N | 100× | — |
| Dif. finitas 2ª ordem, 3D + CFL | *h*² | ~100× | arredondamento acumulado |
| Sistemas caóticos | — | — | horizonte de Lyapunov |
| Escala de LLMs | *C*⁻⁰·⁰⁵ | ~10²⁰ | perda irredutível dos dados |

## 7. Conclusões

1. **A acurácia melhora com mais processamento, com retorno geometricamente
   decrescente** — custo linear em N, ganho logarítmico.
2. **A ordem de convergência domina a contagem de núcleos** — 14 casas: ~24 h em 1
   núcleo, ~3,6 h em 20 threads, **0,0001 s** trocando de série.
3. **Existe um teto absoluto, e ele é da representação numérica** — 999 milhões de
   iterações, ~0,95 s de CPU, resultado idêntico bit a bit.
4. **Paralelizar altera os últimos dígitos**, porque a soma de ponto flutuante não é
   associativa.
5. **O padrão se repete em simulação física e em IA** — lei de potência no custo e um
   piso que o compute não atravessa.

## 8. Reprodução e leitura da saída

```bash
gcc -O2 -Wall -Wextra -std=c99 pi_serie.c -o pi_serie.exe -lm
./pi_serie.exe 6     # instantâneo      ./pi_serie.exe     # até 10^9, ~2 s
```

Na coluna `ERRO ABS`, as linhas do Nilakantha a partir de N = 10⁶ repetem
`6.217e-15` inalterado **enquanto `T.PAREDE` cresce 10× por linha** — as duas colunas
lado a lado são a Seção 5.3 em uma tela.

## 9. Glossário dos termos empregados

| termo | definição adotada, com o valor medido |
|---|---|
| **Ordem de convergência** | expoente com que o erro decai com o número de termos. Leibniz: 1ª ordem (1/N). Nilakantha: 3ª ordem (1/4N³) |
| **Erro de truncamento** | erro por parar a série num termo finito. **Domina em Leibniz** em toda a faixa |
| **Erro de arredondamento** | erro por precisão finita das operações. **Domina em Nilakantha** a partir de N ≈ 10⁵ |
| **IEEE-754 dupla precisão** | o `double`: 64 bits, **53 bits de mantissa**, ~15–17 dígitos decimais |
| ***ulp*** | distância entre dois `double` consecutivos. Perto de 3,14: 2⁻⁵¹ ≈ **4,44 × 10⁻¹⁶** |
| **Épsilon de máquina** | 2⁻⁵² ≈ 2,22 × 10⁻¹⁶, o menor ε com 1 + ε ≠ 1 |
| **Absorção / estagnação** | termo pequeno demais frente ao acumulador é arredondado de volta. A partir de *k* ≈ **1,3 × 10⁵** nenhum bit muda |
| **Não associatividade** | (*a*+*b*)+*c* ≠ *a*+(*b*+*c*); é por isso que o número de threads muda os últimos dígitos |
| **Somatório de Kahan** | mantém variável de compensação e recupera os bits perdidos; eleva o teto sem trocar o tipo |
| **Speedup / eficiência** | *t*(1)/*t*(*p*) e *speedup*/*p*. Medidos: **6,71×** e **37%** com 18 threads |
| **Lei de Amdahl** | teto de 1/*f*, com *f* a fração serial |
| **Métrica de Karp-Flatt** | estimativa empírica de *f*. Medida **0,08–0,13, crescendo com *p*** → overhead, não código serial |
| **`reduction`** | cada thread acumula em cópia privada e o compilador combina no fim — correção por **privatização** |
| **Seção crítica** | serializa o acesso; igualmente correta e **67× mais lenta** que `reduction` |
| **Condição de corrida** | escrita concorrente sem sincronização. Sem `reduction`, π é destruído e muda a cada execução |
| **`static` / `dynamic` / `guided`** | políticas de distribuição das iterações; `dynamic` recupera parte do desbalanceamento |
| **Núcleos heterogêneos** | 6 P-cores + 8 E-cores: fatias iguais deixam o P-core esperando o E-core |
| **Hyperthreading** | duas threads lógicas por núcleo físico **compartilhando a unidade divisora** — o gargalo deste laço |
| **Tempo de parede × de CPU** | relógio decorrido × soma do tempo dos núcleos ocupados; coincidem no serial, descolam no paralelo |
| **Condição CFL** | amarra o passo de tempo ao da malha (Δ*t* ∝ *h*); faz o refino custar ~100× por casa decimal |
| **Expoente de Lyapunov** | taxa de crescimento de perturbações em sistema caótico; impõe horizonte de previsibilidade |
| **Leis de escala / perda irredutível** | *L* ∝ *C*⁻⁰·⁰⁵ e o piso dado pela entropia dos dados — o análogo do *ulp* |

## 10. Questões metodológicas

| pergunta | resposta |
|---|---|
| Por que duas séries, e não uma? | Com uma, mediria-se apenas que o erro cai. Com duas de ordens diferentes e **mesmo custo O(N)**, mede-se *quanto* cada iteração compra — isola a ordem de convergência |
| Por que não `M_PI`? | Não é padrão C, e a referência precisa de mais dígitos que o `double` guarda, senão o erro medido inclui o erro da referência |
| Por que o fator é exatamente 10,0? | Porque o erro é ∝ 1/N e N foi multiplicado por 10 em cada passo. Os oito passos com 10,0 validam a medição |
| Por que só o Nilakantha satura? | Só ele **alcança o piso**: em N = 10⁵ o erro já é da ordem do *ulp*. Leibniz, em 10⁹, ainda está em 10⁻⁹ — seis ordens acima do piso |
| E com `long double`? | O teto **sobe**, o fenômeno permanece: é da representação, não do algoritmo |
| Como foi paralelizado? | `#pragma omp parallel for reduction(+:soma)` em `pi_omp.c`. Por seção crítica também é correto e custa 67× mais |
| Qual o *speedup* teórico? | Karp-Flatt deu fração serial 0,08–0,13 crescente com *p*; Amdahl dá teto de 8× a 12× e o medido foi 6,71× |
| Por que a eficiência cai a 37%? | Núcleos heterogêneos desbalanceiam, e as irmãs de hyperthreading disputam a unidade divisora — que é o limite deste laço |
| Qual a conclusão prática? | Antes de pedir mais núcleos, verificar **a ordem de convergência do método** e **a tolerância que o problema exige** |

<div class="quebra"></div>

## 11. Código-fonte

{{CODIGO:pi_serie.c}}
