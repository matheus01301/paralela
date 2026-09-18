# Tutoria - Tarefa 11

## Etapa atual

Interpretação do enunciado. O código ainda não foi escrito.

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

## Expectativa de desempenho

O trabalho por célula é uniforme. Por isso, `static` provavelmente será mais
rápido que `dynamic` e `guided`, pois não precisa redistribuir trabalho durante o
laço. Essa é apenas a hipótese; a conclusão deverá usar os tempos medidos.
