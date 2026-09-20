# Guia de apresentação — Tarefa 11

## Resumo em 30 segundos

Simulei um fluido só com viscosidade. Sem pressão e sem advecção, Navier-Stokes
vira difusão. Diferenças centrais no espaço, Euler explícito no tempo, bordas
periódicas, dois estados em memória. Validei e paralelizei com `parallel for`,
comparando `static`, `dynamic` e `guided`, com e sem `collapse(2)`.

## Números de cor

| | |
|---|---|
| estabilidade | `r = ν·Δt/h² ≤ 1/4` — usei 0,20 |
| melhor | `static` sem collapse: 0,0218 s, **32,1×** |
| pior | `dynamic` + `collapse(2)`: 7,09 s, **0,10×** |
| iterações | 1.024 sem collapse, 1.048.576 com |

## As partes, na ordem

**1. A equação.** Tirei pressão, advecção e forças externas. Sobrou
`∂u/∂t = ν∇²u` para cada componente, que é difusão.

**2. `atualiza_celula`.** Laplaciano dos 4 vizinhos menos 4× o centro, dividido
por `h²`. Depois Euler: novo = atual + `ν·Δt·`laplaciano.

**3. Os `%` das bordas.** Bordas periódicas. Sem isso o campo constante mudaria
na borda e a validação não funcionaria.

**4. As quatro matrizes.** Leio de `u`,`v` e escrevo em `un`,`vn`. Numa matriz só,
o resultado dependeria da ordem da varredura — e em paralelo seria corrida. No
fim do passo troco os ponteiros, não copio.

**5. Os quatro testes.** Parado e constante dão zero **exato**. Perturbação cai
de 2,000000 para 1,001986 e a soma se conserva. Com `r = 0,30` o pico vai a
3,1e+26: diverge, como a teoria manda.

**6. As sete funções e a tabela.** Mesmo corpo, só a diretiva muda. `static`
ganhou em tudo.

## Se perguntarem

**Por que `static` ganhou?** Toda célula custa igual. Não há desbalanceamento a
corrigir, então vence quem tem menos custo de controle.

**Por que `dynamic` + `collapse(2)` quebrou?** `collapse` leva de 1.024 para
1.048.576 iterações e o bloco padrão do `dynamic` é 1. Dá ~210 milhões de
entregas sincronizadas, uma por célula.

**`collapse` é ruim?** Não, é inadequado aqui. Ele serve quando falta iteração no
laço externo. Tenho 1.024 linhas para 64 threads — sobra. Só paga indexação.

**`dynamic` melhora com mais threads?** Não: 0,2290 s com 16 e 0,2866 s com 64.
Mais threads disputam o mesmo contador do laço. Só `static` escalou de verdade —
e repetindo o experimento só os tempos de `static` se reproduziram dentro de 1%.

**Precisou de `critical` ou `reduction`?** Não. Cada iteração escreve numa
posição diferente e só lê do estado anterior.

**Localidade espacial.** Matriz *row-major*, `j` no laço interno: endereços
consecutivos, e os 8 doubles de cada linha de cache são usados. Invertendo os
laços, eu usaria 1 de cada 8.

**Localidade temporal.** Cada valor é lido 5 vezes — uma como centro, quatro como
vizinho. A reutilização acontece a poucas iterações de distância, então ainda
está no cache.

**Por que o speedup é sublinear?** 50% de eficiência em 64 threads. É limitado
por memória: 32 MB por passo, muito além do cache, e poucas contas por byte.

**Você otimizou?** Não, de propósito. O enunciado pede para *explorar o impacto*
das cláusulas. Otimizar o laço mediria a minha otimização, não as cláusulas.
