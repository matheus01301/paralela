# Tutoria - Tarefa 11

## Etapa atual

Atividade concluída: código, medições no NPAD, relatório em PDF e guia de
defesa. Executada em ritmo de entrega, não de tutoria passo a passo — o aluno
precisou enviar 11, 12 e 13 rapidamente e vai estudar depois.

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
