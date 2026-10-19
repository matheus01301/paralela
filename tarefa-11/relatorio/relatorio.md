# Difusão viscosa com OpenMP: schedule e collapse

## O que foi feito

Simulei o movimento de um fluido com a equação de Navier-Stokes considerando **só
a viscosidade**, sem pressão e sem forças externas. Validei a simulação com três
testes e depois paralelizei com OpenMP, comparando `schedule(static)`,
`schedule(dynamic)` e `schedule(guided)`, cada um com e sem `collapse(2)`.

**Resultado:** `static` sem `collapse` foi o melhor em todos os casos (até 42×).
`dynamic` com `collapse(2)` foi **9× mais lento que o código sequencial**.

## O modelo

Sem pressão e sem forças externas, sobra só a viscosidade. Cada componente da
velocidade (u na horizontal, v na vertical) vira uma equação de **difusão**:

<div class="formula">∂u/∂t = ν ∇²u &nbsp;&nbsp;&nbsp;&nbsp; ∂v/∂t = ν ∇²v</div>

- **Espaço:** malha n × n, com o laplaciano por **diferenças finitas**, usando os 4
  vizinhos: `(cima + baixo + esquerda + direita − 4·centro) / h²`.
- **Tempo:** Euler explícito: `novo = atual + ν·Δt·∇²(atual)`.
- **Estabilidade:** esse método só é estável se `r = ν·Δt/h² ≤ 1/4`. Usei `r = 0,20`.
- **Bordas periódicas:** a malha "dá a volta" (o `%` no código).

> **Como eu explico:** o laplaciano compara a célula com os vizinhos. Se ela está
> maior que eles, diminui; se está menor, aumenta. Por isso uma perturbação vai se
> espalhando e o campo fica uniforme, que é o efeito da viscosidade.

## Como funciona o código

| parte | o que faz |
|---|---|
| `u`, `v` e `un`, `vn` | passo atual e próximo passo. O próximo é calculado lendo só o atual |
| `atualiza_celula(i, j)` | a conta de uma célula, com o `%` para dar a volta nas bordas |
| `passo(versao)` | um passo inteiro; as 7 versões têm o **mesmo laço**, só a diretiva muda |
| `simular` | repete os passos e troca os ponteiros (o próximo vira o atual), sem copiar dados |
| `medir` | menor tempo de 5 execuções, com `omp_get_wtime` |
| `memcmp` | confere se cada versão paralela deu **exatamente** o mesmo resultado do sequencial |

> **Como eu explico:** uso duas cópias porque cada célula precisa dos vizinhos
> antigos. Se eu escrevesse por cima, uma célula já atualizada entraria na conta da
> vizinha, e em paralelo isso seria condição de corrida. Com duas cópias, nenhuma
> thread escreve onde outra lê, então não preciso de `critical` nem `atomic`.

## Validação

Malha 1024 × 1024, 200 passos, sequencial:

| teste | esperado | obtido |
|---|---|---|
| fluido parado (u = 0, v = 0) | continua parado | desvio = 0 |
| velocidade constante (u = 2, v = −1) | não muda | desvio = 0 |
| perturbação no centro | o pico diminui e se espalha | pico 2,000 → 1,002 |
| soma de todas as células | se conserva (nada entra ou sai) | 1048577 → 1048577 |

As 6 versões paralelas deram resultado **idêntico byte a byte** ao sequencial, em
todas as quantidades de threads.

## Conceitos (slide 37)

| cláusula | como divide as iterações |
|---|---|
| `schedule(static)` | divide em blocos iguais **antes** de começar. Sem custo durante a execução |
| `schedule(dynamic)` | cada thread **pede** o próximo bloco quando termina o seu. Padrão: bloco de **1** iteração |
| `schedule(guided)` | como o dynamic, mas os blocos começam grandes e vão **diminuindo** |
| chunksize | tamanho do bloco: `schedule(dynamic, 64)`, por exemplo. Aqui usei o padrão |
| `collapse(2)` | junta os dois laços (i e j) em um só: 1024 × 1024 = 1.048.576 iterações para dividir |

## Resultados

NPAD, nó `r2n20` exclusivo, GCC 8.5.0, `-O2`, threads fixas em núcleos
(`OMP_PLACES=cores`, `OMP_PROC_BIND=close`). Malha 1024 × 1024, 200 passos, menor
tempo de 5 execuções. Sequencial: **0,83 s**. Tempo em segundos e speedup:

| versão | 16 threads | 32 threads | 64 threads | 128 threads |
|---|---:|---:|---:|---:|
| `static` | **0,0545** (15,2×) | **0,0290** (28,6×) | **0,0196** (42,3×) | **0,0281** (29,6×) |
| `static` + `collapse(2)` | 0,1062 (7,8×) | 0,0543 (15,3×) | 0,0310 (26,8×) | 0,0295 (28,2×) |
| `dynamic` | 0,2343 (3,5×) | 0,3051 (2,7×) | 0,3687 (2,3×) | 0,8141 (1,0×) |
| `dynamic` + `collapse(2)` | 7,7074 (0,11×) | 7,3814 (0,11×) | 7,4351 (0,11×) | 8,9711 (0,09×) |
| `guided` | 0,1387 (6,0×) | 0,2091 (4,0×) | 0,2662 (3,1×) | 0,5473 (1,5×) |
| `guided` + `collapse(2)` | 0,1536 (5,4×) | 0,2161 (3,9×) | 0,2755 (3,0×) | 0,4919 (1,7×) |

## Análise

**1. `static` é o melhor porque todas as células custam igual.** Toda célula faz a
mesma conta. Não há desbalanceamento para corrigir, então dividir em blocos iguais
antes de começar é o ideal, e as threads não precisam combinar nada durante a
execução. Com 16 threads a eficiência foi de 0,95.

**2. `dynamic` piora com mais threads.** Com bloco 1, cada linha é um pedido: 1024
pedidos por passo, todas as threads disputando o mesmo contador. Quanto mais
threads, mais disputa: foi de 3,5× (16 threads) para 1,0× (128). E cada linha pega
é de um lugar diferente da matriz, o que atrapalha o cache.

**3. `guided` fica no meio.** Começa com blocos grandes e faz menos pedidos que o
`dynamic`, mas no fim os blocos ficam pequenos e a disputa volta. Melhor que
`dynamic`, pior que `static`.

**4. `dynamic` + `collapse(2)` é o pior caso.** Com o `collapse`, são 1.048.576
iterações por passo, e com bloco 1 cada **célula** vira um pedido: mais de 200
milhões de pedidos na simulação. Pedir a célula custa mais do que calculá-la: 9×
mais lento que o sequencial.

**5. `collapse(2)` não ajudou.** Ele serve quando o laço de fora tem poucas
iterações para as threads. Aqui são 1024 linhas para no máximo 128 threads, então
já sobra trabalho. O que ele traz é custo: o programa precisa recalcular `i` e `j`
a partir de um índice só, a cada iteração. Com `static` e 16 threads, ficou 2× mais
lento.

**6. Com 128 threads o `static` piorou.** São só 8 linhas por thread, e a cada um
dos 200 passos as 128 threads precisam ser sincronizadas. O trabalho de cada uma
ficou pequeno perto desse custo fixo, que é a sobrecarga de paralelização.

> **Como eu explico:** o `static` ganhou porque toda célula custa a mesma coisa,
> então dividir igual antes de começar é o melhor. O `dynamic` só vale a pena quando
> as iterações têm custos diferentes. Aqui ele só adicionou o custo de pedir trabalho
> a cada linha. E com `collapse`, esse pedido passa a ser a cada célula, por isso
> ficou mais lento até que o sequencial.

## Conclusão

- **Use `static`** quando as iterações custam igual. É o caso deste problema.
- **`dynamic` e `guided`** servem para iterações com custos diferentes. Aqui só
  adicionaram custo. Com bloco 1, o custo de pedir trabalho supera o de calcular.
- **`collapse`** serve quando o laço de fora tem iterações de menos para as threads.
  Não é o caso: 1024 linhas já bastam.

## Código

{{CODIGO:difusao_viscosa.c}}
