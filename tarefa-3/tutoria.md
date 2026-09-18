# Tutoria — Tarefa 3

## Referência

- Enunciado: [catálogo geral](../enunciados_tarefas_1_a_10.md)
- Código principal: `ilp.c`
- Apoio: `README.md`, `guia_apresentacao.md` e `relatorio/relatorio.md`

## Estado

Os arquivos reais foram revisados e a primeira defesa foi concluída. É necessário
revisar o conceito central de ILP e entender por que os múltiplos acumuladores
ajudam sem criar threads.

## Pontos que precisarão ser verificados

- Paralelismo ao nível de instrução (ILP).
- Dependência carregada pelo laço na soma acumulativa.
- Uso de múltiplos acumuladores para quebrar a cadeia de dependências.
- Efeito real de `-O0`, `-O2` e `-O3` nos resultados medidos.

## Próximo passo

Revisar execução fora de ordem, cadeias de dependência independentes, bolhas no
pipeline e a diferença entre ILP, SIMD e paralelismo entre threads.

## Avaliações

### Defesa de 15/09/2026

**Nota: 4,5/10.**

Acertos demonstrados:

- identificou que as iterações da inicialização são independentes;
- localizou a dependência na soma com um único acumulador;
- reconheceu que as versões múltiplas dividem a soma em cadeias independentes;
- corrigiu a afirmação inicial de que haveria menos operações.

Erros ou lacunas observados:

- inverteu a expansão de ILP; o termo correto é paralelismo ao nível de instrução;
- chamou a dependência de contenção, embora o problema principal seja a latência
  da cadeia de dependências e as bolhas no pipeline;
- descreveu apenas quatro acumuladores, mas o programa compara 2, 4 e 8;
- atribuiu o ganho à redução de operações, embora a quantidade total não diminua;
- afirmou que o Windows dividiria ou reutilizaria threads para executar as cadeias;
- não reconheceu que `soma_multipla4` é executada por uma única thread e que o
  paralelismo vem de instruções independentes sobrepostas dentro do núcleo;
- a defesa terminou antes de demonstrar domínio sobre `-O0`, `-O2`, `-O3`,
  vetorização, saturação em quatro acumuladores e metodologia de medição.

A defesa precisa ser repetida após a revisão.

### Segunda defesa de 15/09/2026

**Nota: 8,0/10.**

Houve melhora clara: o aluno distinguiu execução com uma thread, múltiplos
acumuladores, saturação de recursos, níveis de otimização e SIMD. Permaneceram
imprecisões ao chamar a dependência de contenção, ao sugerir que os acumuladores
eliminam totalmente suas dependências internas e ao dizer que SIMD realiza toda
a soma de uma vez. O correto é falar em cadeia de dependência, várias cadeias
menores e processamento de grupos de dados por instrução vetorial.
