# Retomar: estudo passo a passo da Tarefa 11

Cole este arquivo como prompt para continuar de onde paramos.

---

## Contexto

Estou estudando as tarefas da Unidade 2 de programação paralela, uma por vez,
começando pela 11. Elas foram entregues às pressas, sem eu entender tudo. Agora
quero aprender cada parte para a explicação oral ao professor. Podemos refazer
o código e o relatório se isso facilitar o entendimento.

Siga o `AGENTS.md` da raiz: uma etapa por vez, explicar o papel de cada
variável, função e diretiva, e parar com uma pergunta no fim de cada etapa. Não
programo em C no dia a dia; sou fluente em TypeScript, então comparações com TS
ajudam.

Na explicação oral eu só tenho em mãos o PDF entregue. Por isso o relatório
precisa conter o apoio de fala: resumo executivo, glossário e perguntas e
respostas.

Arquivos da tarefa: `tarefa-11/difusao_viscosa.c`, `resultados.txt`,
`relatorio/relatorio.md` (gera o PDF com `build_pdf.py`), `guia_apresentacao.md`,
`tutoria.md`. O enunciado está em `tarefa-11/PROMPT-CONTINUACAO.md`.

## Plano do estudo

1. ~~O que o enunciado pede~~ ✓
2. ~~O modelo: Navier-Stokes só com viscosidade vira difusão; o laplaciano~~ ✓
3. ~~Euler explícito e o limite `r = ν·Δt/h² ≤ 1/4`~~ ✓
4. **O código sequencial** ← *estamos aqui*
   - 4a. struct `Malha`, grade como vetor único `i*n+j`, vizinhos com `%` e
     bordas periódicas: **explicado, falta eu responder a pergunta**
   - 4b. as quatro matrizes (`u`, `v`, `un`, `vn`), por que não atualizar no
     mesmo vetor, e a troca de ponteiros em `simular` (`double **`)
5. As validações: por que parado/constante dão zero *exato*, conservação da
   soma, o teste com `r = 0,30`
6. OpenMP: por que não há condição de corrida, `static` / `dynamic` / `guided`,
   tamanho de bloco padrão, `collapse(2)`
7. Os resultados: ler cada número da tabela
8. Simplificar o código, reescrever o relatório e simular a defesa

## O que já entendi

- **Viscosidade suaviza diferenças de velocidade**; sem diferença, nada muda.
  Por isso o campo parado ou constante tem que continuar igual.
- A advecção sai junto com a pressão e as forças porque o enunciado manda
  considerar **apenas** a viscosidade.
- **Laplaciano** = `(dir + esq + cima + baixo − 4·centro) / h²`. Ele compara o
  centro com a **média** dos vizinhos, não com cada vizinho (centro 3 com
  vizinhos 5, 5, 1, 1 dá zero).
- Atualização: `novo = atual + r · (vizinhos − 4·centro)`.
- **Limite 1/4**: abrindo a conta, `novo = (1−4r)·centro + r·(cada vizinho)`, uma
  média ponderada. Se o peso do centro ficar negativo (`r > 1/4`), a correção
  passa do ponto, oscila e explode (teste 4: pico 3,1e+26).
- `Δt = r·h²/ν`: mais viscosidade → `Δt` menor; grade mais fina → `Δt` menor com
  o quadrado de `h`.

## Onde errei (revisar na simulação da defesa)

- Achei que dobrar `n` **aumentaria** `Δt`. Confundi `n` (número de pontos) com
  `h` (distância entre eles, `h = 1/n`). O certo: `n` dobra → `h` cai pela
  metade → `Δt` fica **4×** menor → 4× mais passos. Dobrar a grade custa 16×
  mais trabalho.
- Ao responder o caso `n = 512`, acertei a direção mas esqueci o fator 4×.

## Pergunta pendente (etapa 4a)

Numa grade 4×4 (`n = 4`), quais são os 4 vizinhos da célula `i = 0, j = 3`
(canto superior direito)? Dar o `(i, j)` de cada um.

## Correções já identificadas para o relatório

- **Os ~290 GB/s do `static` não podem vir da DRAM.** As matrizes são
  inicializadas por uma thread só, então por *first touch* ficam num único
  domínio NUMA, que não entrega essa banda. A explicação provável: os 32 MB de
  dados cabem no L3 agregado dos núcleos usados, e o `static` dá **as mesmas
  linhas para a mesma thread a cada passo**, então os dados continuam no cache
  dela. `dynamic` e `guided` redistribuem as linhas a cada passo e perdem essa
  afinidade, um motivo de lentidão que o relatório não cita. Confirmar antes de
  escrever.
- O relatório não tem resumo executivo, glossário nem perguntas e respostas.
- `teste_acima_do_limite` não foi pedido pelo enunciado. Decidir se fica (ele
  justifica a escolha de `r = 0,20`).
