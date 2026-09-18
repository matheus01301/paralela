# Tutoria — Tarefa 1

## Referência

- Enunciado: [catálogo geral](../enunciados_tarefas_1_a_10.md)
- Código principal: `pi_serie.c`
- Apoio: `README.md`, `guia_apresentacao.md` e `relatorio/relatorio.md`

## Estado

Os arquivos reais foram revisados e a primeira defesa foi concluída. É necessário
revisar a medição com `clock()` e o cálculo do erro absoluto antes de uma nova
defesa.

## Pontos que a defesa deve cobrir

- Série de Leibniz e alternância de sinais.
- Papel de `2 * i + 1` na geração dos denominadores ímpares.
- Testes com 10⁶, 10⁷, 10⁸ e 10⁹ iterações.
- Erro absoluto calculado por `fabs(PI_REAL - pi)`.
- Relação observada: dez vezes mais iterações, aproximadamente dez vezes menos
  erro e dez vezes mais tempo.
- Medição com `clock()` e comparação com aplicações reais.
- A entrega principal é sequencial; `pi_omp.c` é experimento adicional.

## Próximo passo

Revisar `clock()`, `CLOCKS_PER_SEC` e `fabs(PI_REAL - pi)`. Depois, treinar uma
defesa curta que diferencie tempo de CPU de tempo de parede e explique por que o
erro absoluto nunca é negativo.

## Avaliações

### Defesa em andamento — 15/09/2026

Na resposta inicial, o aluno:

- identificou a série de Leibniz e a alternância entre soma e subtração;
- relacionou mais iterações a menor erro e maior tempo;
- afirmou crescimento de aproximadamente 10 vezes no tempo quando as iterações
  crescem 10 vezes;
- relacionou o compromisso entre processamento e qualidade a épocas de
  treinamento em inteligência artificial.

Pontos que a banca ainda precisa verificar:

- se as relações de tempo e erro são exatas ou aproximadas;
- se aumentar indefinidamente as épocas sempre melhora um modelo;
- compreensão do cálculo dos termos, do erro e da medição de tempo no código.

Na primeira contestação, o aluno reconheceu que mais épocas não garantem melhora
e que existe risco de piora. Entretanto, atribuiu a aquisição de conhecimento ao
usuário; ainda é necessário distinguir usuário, modelo, dados de treinamento e
capacidade de generalização.

### Resultado final

**Nota: 6,5/10.**

Acertos demonstrados:

- identificou a série de Leibniz e a alternância entre soma e subtração;
- explicou `2 * i + 1` como gerador dos denominadores ímpares;
- relacionou mais iterações a menor erro e maior tempo;
- corrigiu a afirmação de que o crescimento de tempo seria exatamente 10 vezes;
- reconheceu que o modelo, e não o usuário, aprende durante o treinamento;
- distinguiu melhora nos dados de treinamento no caso de overfitting;
- reconheceu que o fator 4 transforma a aproximação de π/4 em π.

Erros ou lacunas observados:

- inicialmente omitiu o fator 4 ao calcular duas iterações;
- não explicou claramente que, no overfitting, o desempenho em dados novos tende
  a piorar mesmo quando o erro de treinamento diminui;
- afirmou incorretamente que `clock()` mede wall time e conta necessariamente o
  tempo em que o processo fica pausado;
- não soube explicar `fabs(PI_REAL - pi)` nem por que o erro absoluto não é
  negativo.

A defesa foi concluída em 15/09/2026.
