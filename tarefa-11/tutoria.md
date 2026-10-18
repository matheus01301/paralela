# Tutoria - Tarefa 11

## Etapa atual

Atividade entregue (código, medições no NPAD, relatório e guia) em ritmo de
entrega. **Estudo passo a passo iniciado em 14/10/2026**, podendo refazer código
e relatório para facilitar o entendimento.

Plano do estudo: (1) enunciado e modelo, (2) Euler explícito e `r <= 1/4`,
(3) código sequencial, (4) validações, (5) OpenMP, (6) resultados,
(7) simplificar código, reescrever relatório com resumo executivo, glossário e
perguntas, e simular a defesa.

Progresso:
- Etapas 1 e 2 concluídas. Entendeu "viscosidade suaviza diferenças", que o
  laplaciano compara o centro com a **média** dos vizinhos, e o limite 1/4 como
  "pesos da média ponderada não podem ficar negativos".
- Errou de início: achou que dobrar `n` aumentaria `dt` (confundiu `n` com `h`).
  Corrigiu: `n` maior → `h` menor → `dt` 4x menor. Revisar na simulação.
- Próximo: ver `RETOMAR-ESTUDO.md` (pergunta pendente sobre vizinhos na grade 4x4).

Correção a fazer no relatório: os ~290 GB/s do `static` não podem vir da DRAM
(first touch deixa tudo num domínio NUMA). A explicação é o conjunto de dados
(32 MB) caber no L3 agregado e o `static` manter as mesmas linhas na mesma
thread a cada passo; `dynamic`/`guided` perdem essa afinidade.

**Para estudar depois:** o `guia_apresentacao.md` tem o roteiro completo. Os
pontos que mais provavelmente caem são o limite `r <= 1/4`, o motivo de existirem
duas matrizes, e por que `dynamic` com `collapse(2)` ficou 10x mais lento que o
sequencial.

## O que a atividade pede

- Simular um campo de velocidade ao longo do tempo.
- Considerar somente o efeito da viscosidade.
- Discretizar o espaço com diferenças finitas.
- Validar primeiro com o fluido parado ou com velocidade constante.
- Inserir uma pequena perturbação e verificar sua difusão.
- Paralelizar os cálculos com OpenMP.
- Comparar cláusulas `schedule` e o uso de `collapse`.
- Entregar um relatório em PDF com o código e realce de sintaxe.

## Modelo matemático adotado

Sem pressão, forças externas e outros efeitos, cada componente da velocidade
obedece a uma equação de difusão:

`du/dt = nu * (d2u/dx2 + d2u/dy2)`

`dv/dt = nu * (d2v/dx2 + d2v/dy2)`

- `u` e `v`: componentes do campo de velocidade em duas dimensões.
- `nu`: viscosidade cinemática.
- O laplaciano mede como o valor de uma célula difere dos seus vizinhos.

## Discretização planejada

- Malha regular bidimensional.
- Diferenças centrais para as derivadas espaciais de segunda ordem.
- Euler explícito para avançar no tempo.
- Dois campos em memória: estado atual e próximo estado.
- Condições de contorno periódicas para que uma velocidade constante possa
  permanecer constante também nas bordas.

## Validações necessárias

1. Campo inicialmente parado: deve continuar com velocidade zero.
2. Campo inicialmente constante: deve conservar o mesmo valor.
3. Perturbação no centro: o pico deve diminuir e se espalhar suavemente.
4. Passo de tempo: deve respeitar o limite de estabilidade do método explícito.

## OpenMP que realmente pertence à atividade

- `parallel for`: dividir as células da malha entre as threads.
- `schedule(static)`, `schedule(dynamic)` e `schedule(guided)`: comparar a forma
  de distribuição das iterações.
- `collapse(2)`: transformar os dois laços da malha em um único espaço de
  iterações para a distribuição do OpenMP.

`sections` e `simd` não serão incluídos inicialmente, pois não foram pedidos e
não são necessários para responder ao enunciado.

## Conceitos para a defesa

- `static`: distribui blocos previamente; é adequado quando todas as células têm
  praticamente o mesmo custo.
- `dynamic`: as threads solicitam novos blocos durante a execução; ajuda no
  desbalanceamento, mas tem maior custo de controle.
- `guided`: começa com blocos maiores e reduz o tamanho dos próximos blocos,
  respeitando o tamanho mínimo configurado.
- `collapse(2)`: combina as iterações de dois laços aninhados; não cria mais
  cálculos, apenas muda o espaço que pode ser distribuído.
- Localidade espacial: células vizinhas estão próximas na memória quando a
  matriz é percorrida na ordem correta.
- Localidade temporal: os valores do estado atual são reutilizados para calcular
  células vizinhas antes de saírem do cache.

## Expectativa de desempenho e o que foi medido

A hipótese era que `static` venceria, porque o trabalho por célula é uniforme e
não há desbalanceamento a corrigir. Confirmou-se.

Medições no NPAD (nó `r2n00` da partição `amd-512`, exclusivo, malha 1024x1024,
200 passos, mediana de 5 execuções):

| schedule | collapse | 16 thr | 32 thr | 64 thr |
|---|---|---:|---:|---:|
| sequencial | - | 0,6973 | 0,6977 | 0,6974 |
| `static` | não | **0,0567** | **0,0315** | **0,0218** |
| `static` | `collapse(2)` | 0,1068 | 0,0558 | 0,0323 |
| `dynamic` | não | 0,2290 | 0,2183 | 0,2866 |
| `dynamic` | `collapse(2)` | 7,4154 | 6,9383 | 7,0872 |
| `guided` | não | 0,1796 | 0,1461 | 0,1397 |
| `guided` | `collapse(2)` | 0,1925 | 0,1502 | 0,1476 |

O que a teoria sozinha não daria:

- `dynamic` **não** melhora com mais threads: 0,2290 s com 16 e 0,2866 s com 64.
  Só `static` escalou de verdade.
- `dynamic` com `collapse(2)` ficou 10x mais lento que o código sequencial:
  bloco padrão 1 sobre 1.048.576 iterações dá ~210 milhões de entregas
  sincronizadas.
- O speedup é sublinear (77% -> 69% -> 50% de eficiência) porque o problema é
  limitado por memória, não por CPU.
- Repetindo o experimento, só `static` se reproduziu dentro de 1%. `guided` em
  64 threads deu 0,2345 s numa rodada e 0,1397 s na seguinte — o custo das
  políticas dinâmicas depende de disputa em tempo de execução. Cuidado para não
  afirmar na defesa que `guided` piora com mais threads: isso era ruído.

Nada foi otimizado de propósito. O gargalo de *first touch* / NUMA foi deixado
no lugar e registrado no relatório, porque corrigi-lo mascararia o efeito que o
enunciado manda observar.
