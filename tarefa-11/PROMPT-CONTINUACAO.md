# Prompt para continuar a Tarefa 11 em outro chat

Estou fazendo atividades de programação paralela e preciso me preparar para uma
defesa oral. Leia primeiro o arquivo `AGENTS.md` da raiz deste repositório e siga
as orientações dele durante toda a conversa.

## Contexto da disciplina

Na Unidade 1, eu tinha dez atividades. O professor sorteou duas delas para uma
defesa oral. Na Tarefa 2, faltaram no relatório conceitos como localidade
temporal e localidade espacial. Na Tarefa 3, tive dificuldade com conhecimentos
básicos, como vetorização, e não consegui apresentar bem.

Para a Unidade 2, não quero que a IA simplesmente faça tudo sozinha. Quero
construir cada atividade por etapas, entendendo o motivo de cada variável,
função, estrutura, biblioteca, diretiva e escolha. Pare em pontos importantes
para que eu possa responder perguntas e discutir a solução antes de continuar.

O código deve conter somente o necessário para o enunciado. Não acrescente
funcionalidades, abstrações, otimizações, comentários ou tecnologias que não
tenham utilidade clara. Antes de finalizar, verifique se cada elemento foi
pedido, se é necessário e se eu consigo explicá-lo.

Os relatórios anteriores ficaram com aparência de texto produzido por IA. O
novo relatório deve ser simples e natural, com poucos títulos, frases curtas,
palavras-chave e apenas os parágrafos necessários. O roteiro da defesa também
deve ser curto e fácil de consultar sem parecer que estou lendo uma resposta.

## Enunciado da Tarefa 11

> Escreva um código que simule o movimento de um fluido ao longo do tempo usando
> a equação de Navier-Stokes, considerando apenas os efeitos da viscosidade.
> Desconsidere a pressão e quaisquer forças externas. Utilize diferenças finitas
> para discretizar o espaço e simule a evolução da velocidade do fluido no
> tempo. Inicialize o fluido parado ou com velocidade constante e verifique se o
> campo permanece estável. Em seguida, crie uma pequena perturbação e observe se
> ela se difunde suavemente. Após validar o código, paralelize-o com OpenMP e
> explore o impacto das cláusulas schedule e collapse no desempenho da execução
> paralela.
>
> O relatório deve ser entregue em PDF e conter o código utilizado, com realce
> de sintaxe.

## Decisões e conceitos discutidos até agora

Adotamos uma malha bidimensional e um campo vetorial com duas componentes:

- `u`: velocidade horizontal;
- `v`: velocidade vertical.

Como o enunciado pede somente o efeito da viscosidade, retiramos advecção,
pressão e forças externas da equação de Navier-Stokes. Cada componente passa a
obedecer a uma equação de difusão:

`du/dt = nu * (d2u/dx2 + d2u/dy2)`

`dv/dt = nu * (d2v/dx2 + d2v/dy2)`

Eu já entendi que, em um campo constante, todas as células têm a mesma
velocidade. Não existe diferença entre uma célula e as vizinhas, as derivadas
espaciais de segunda ordem são zero, o laplaciano é zero e a velocidade não muda
com o tempo.

A atualização por diferenças finitas e Euler explícito foi apresentada como:

`u_novo = u_atual + nu * dt * laplaciano(u_atual)`

Quando `dx = dy = h`:

`laplaciano = (direita + esquerda + cima + baixo - 4 * centro) / h^2`

No exemplo em que a célula central vale 8 e seus quatro vizinhos valem 5, o
laplaciano é negativo. Por isso, o valor central diminui e a perturbação começa
a se difundir.

Precisaremos de quatro matrizes:

- `u_atual` e `u_novo`;
- `v_atual` e `v_novo`.

O novo estado deve ser calculado lendo somente o estado atual. Atualizar tudo na
mesma matriz faria o resultado depender da ordem do laço. Em paralelo, uma
thread também poderia ler uma posição enquanto outra estivesse escrevendo nela,
causando uma condição de corrida. Depois de cada passo, os ponteiros das
matrizes atual e nova podem ser trocados.

Também discutimos o limite de estabilidade do Euler explícito em duas dimensões,
quando os espaçamentos são iguais:

`r = nu * dt / h^2 <= 1/4`

Eu inicialmente respondi de forma errada que aumentar a viscosidade permitiria
aumentar `dt`, mas já corrigi esse entendimento. A forma que memorizei foi:

**maior viscosidade -> difusão mais rápida -> menor passo de tempo.**

Escolhemos condições de contorno periódicas. As bordas opostas ficam conectadas,
evitando introduzir paredes que não foram pedidas e permitindo que um campo
constante permaneça estável também nas bordas.

## OpenMP discutido

- `parallel for` será usado para dividir as células entre as threads.
- `static` distribui as iterações antecipadamente e tende a ser adequado porque
  cada célula tem praticamente o mesmo custo.
- `dynamic` entrega novos blocos conforme as threads terminam os anteriores,
  adicionando custo de controle.
- `guided` começa com blocos maiores e reduz gradualmente os próximos blocos,
  respeitando o tamanho mínimo configurado.
- `collapse(2)` combina o espaço de iterações de dois laços aninhados. Ele não
  aumenta o número de cálculos.
- O desempenho de `static`, `dynamic`, `guided` e `collapse(2)` deve ser medido;
  não devemos afirmar o vencedor apenas pela teoria.
- Localidade espacial: células vizinhas ficam próximas na memória quando a
  matriz é percorrida na ordem correta.
- Localidade temporal: valores do estado atual são reutilizados no cálculo de
  células vizinhas enquanto ainda podem estar no cache.

Não inclua `sections` nem `simd` inicialmente. Essas construções apareceram nas
anotações da aula, mas não são necessárias para responder ao enunciado. Se alguma
delas for sugerida depois, explique primeiro por que seria realmente necessária.

## Ponto exato em que paramos

Ainda não existe código da simulação. Já entendemos o modelo matemático, a
atualização por diferenças finitas, a necessidade de dois estados, o limite de
estabilidade e a escolha das bordas periódicas.

O próximo passo deve ser explicar como representar a malha na memória e montar
um pseudocódigo sequencial pequeno. Não escreva a atividade completa de uma vez.
Depois, conduza nesta ordem:

1. implementação sequencial mínima;
2. teste do campo parado ou constante;
3. teste da perturbação;
4. confirmação da estabilidade numérica;
5. paralelização com OpenMP;
6. experimentos com `schedule` e `collapse`;
7. relatório simples em PDF com código realçado;
8. roteiro curto e simulação da defesa oral.

Continue a tutoria fazendo perguntas curtas para confirmar que eu entendi cada
parte antes de avançar.
