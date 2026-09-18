# Tutoria — Tarefa 6

## Referência

- Enunciado: [catálogo geral](../enunciados_tarefas_1_a_10.md)
- Código principal: `pi_monte_carlo.c`
- Apoio: `README.md`, `guia_apresentacao.md`, `resultados.txt` e relatório.

## Estado

Estudo conceitual e primeira defesa concluídos. O aluno demonstrou domínio geral
da corrida, das cláusulas de escopo e da privatização, mas precisa distinguir com
mais precisão as variáveis usadas em cada uma das quatro versões.

## Conteúdo já discutido

- Estimativa de π por Monte Carlo.
- Condição de corrida em `dentro++`.
- `critical` por ponto e seu alto custo.
- Contador privado com apenas um `critical` por thread.
- Cláusulas `private`, `firstprivate`, `lastprivate` e `shared`.
- `default(none)` como forma de tornar o escopo explícito.
- Barreiras, `atomic` e `reduction` como alternativas conceituais.

## Pontos a observar na defesa

- Explicar com precisão por que `dentro++` não é atômico.
- Não confundir escopo de variável com sincronização.
- Distinguir correção de desempenho.
- Basear números e identificadores nos arquivos reais.

## Avaliações

### Defesa de 15/09/2026

**Nota: 8,0/10.**

Acertos demonstrados:

- explicou o teste geométrico do Monte Carlo e o fator 4;
- identificou a corrida no incremento do contador compartilhado;
- explicou corretamente `default(none)`, `firstprivate`, `private` e
  `lastprivate` em termos gerais;
- distinguiu a última iteração lógica da última thread a terminar;
- posicionou o `critical` final depois do `omp for`, ainda dentro de `parallel`;
- explicou que a versão privada faz uma combinação protegida por thread e que,
  com 20 threads, são 20 atualizações.

Erros ou imprecisões observados:

- descreveu de forma imprecisa a geometria como “área de um quarto de pi”; o
  correto é dizer que a razão entre a área do círculo e a do quadrado é π/4;
- falou em perda de iterações, mas a corrida perde incrementos de `dentro`;
- sugeriu convergência da versão com corrida, embora ela produza uma contagem
  variável e artificialmente baixa;
- afirmou que todas as variáveis `private` precisam ser inicializadas com zero;
  somente `dentro_local` precisa disso, enquanto `i`, `x` e `y` recebem valores
  antes de serem usados;
- confundiu as versões ao dizer que `dentro_local` era compartilhada no
  `critical` por ponto; essa versão usa o contador compartilhado `dentro` e nem
  possui `dentro_local`;
- chamou a execução do `critical` de uma vez por ponto, quando o bloco só é
  visitado nos pontos aceitos, aproximadamente 7,85 milhões de hits;
- não apresentou os tempos medianos nem a versão sequencial como referência.

A defesa foi concluída com pontos específicos para revisão.
