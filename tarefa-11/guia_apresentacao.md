# Guia de apresentação — Tarefa 11

## Resumo em 30 segundos

Simulei um fluido só com viscosidade: sem pressão, Navier-Stokes vira difusão.
Diferenças finitas no espaço, Euler explícito no tempo, bordas periódicas. Validei
com 3 testes e comparei `static`, `dynamic` e `guided`, com e sem `collapse(2)`.
`static` ganhou sempre; `dynamic` + `collapse(2)` ficou 9× mais lento que o
sequencial.

## Números de cor

| | |
|---|---|
| estabilidade | `r = ν·Δt/h² ≤ 1/4`, usei 0,20 |
| sequencial | 0,83 s (1024², 200 passos) |
| melhor | `static`, 64 threads: 0,0196 s, **42×** |
| pior | `dynamic` + `collapse(2)`: ~7,5 s, **0,11×** |
| iterações | 1.024 sem `collapse`, 1.048.576 com |

## Se perguntarem

**Por que Navier-Stokes virou difusão?** Sem pressão e sem forças externas sobra só
o termo da viscosidade: ∂u/∂t = ν∇²u.

**O que é o laplaciano no código?** Cima + baixo + esquerda + direita − 4·centro,
dividido por h². Compara a célula com os vizinhos.

**Por que r ≤ 1/4?** É o limite de estabilidade do Euler explícito em 2D. Acima
disso a simulação explode. Usei 0,20 para ficar abaixo.

**Como você validou?** Fluido parado continua parado; velocidade constante não muda;
uma perturbação se espalha e a soma se conserva.

**Por que duas cópias (u e un)?** Cada célula precisa dos vizinhos antigos. Com uma
cópia só, uma célula já atualizada entraria na conta da vizinha, e em paralelo seria
condição de corrida.

**Por que não precisa de critical ou atomic?** Nenhuma thread escreve onde outra lê:
todas leem de `u` e escrevem em `un`, cada uma nas suas células.

**Diferença entre static, dynamic e guided?** `static` divide em blocos iguais antes
de começar. `dynamic` dá um bloco por vez a quem pedir (padrão: 1 iteração).
`guided` dá blocos que começam grandes e vão diminuindo.

**Por que static ganhou?** Todas as células custam igual, então não há
desbalanceamento. Dividir igual antes é o ideal, sem custo durante a execução.

**Quando dynamic seria melhor?** Quando as iterações têm custos diferentes. Aí quem
termina antes pega mais trabalho.

**Por que dynamic + collapse foi tão ruim?** Com `collapse`, cada célula vira uma
iteração (1 milhão por passo), e com bloco 1 cada célula vira um pedido. Pedir custa
mais que calcular.

**Como melhoraria o dynamic?** Com um chunksize maior, como `schedule(dynamic, 64)`:
menos pedidos. (Não medi.)

**Para que serve o collapse?** Quando o laço de fora tem poucas iterações para as
threads. Aqui são 1024 linhas para no máximo 128 threads, já sobra trabalho.

**Por que com 128 threads o static piorou?** Só 8 linhas por thread, e a cada passo
as 128 threads se sincronizam. O custo fixo pesou mais que o trabalho.

**O que são `sections` e `simd` (slide 37)?** `sections` divide **blocos de código
diferentes** entre threads, em vez de iterações de um laço. `simd` pede para o
compilador vetorizar o laço: uma instrução processa vários dados de uma vez.

## Detalhes do código

**Por que `static inline` na função da célula?** `inline` pede para o compilador
colar o corpo da função dentro do laço, sem fazer uma chamada por célula. Sem ele o
sequencial ficou 2× mais lento (testei).

**Por que variáveis globais (`n`, `fator`, `u`, `v`...)?** Para não passar os mesmos
6 valores para toda função. Dentro do laço paralelo todas as threads só leem `u` e
`v` e escrevem em `un` e `vn`; os ponteiros só trocam fora do laço.

**Por que um `switch`?** Cada `case` é uma versão: mesmo laço, diretiva diferente.

**Por que comparar com `memcmp` (byte a byte)?** Não há soma entre threads: cada
célula faz as mesmas contas na mesma ordem, seja qual for a thread. Então o
resultado tem que ser idêntico.

**Por que o menor de 5 tempos?** O menor é o que sofreu menos interferência do
sistema.

**`omp_get_wtime`?** Relógio do OpenMP, em segundos.

**`--exclusive` e `OMP_PROC_BIND=close` no script?** Nó só meu e cada thread presa
num núcleo, para só o `schedule` mudar entre as medições.

## Comandos

```bash
ssh npad
cd ~/paralela/tarefa-11
sbatch job_npad.sh

cd tarefa-11/relatorio
python build_pdf.py
```
