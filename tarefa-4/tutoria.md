# Tutoria — Tarefa 4

## Referência

- Enunciado: [catálogo geral](../enunciados_tarefas_1_a_10.md)
- Códigos principais: `memory_bound.c` e `cpu_bound.c`
- Apoio: `README.md`, `guia_apresentacao.md`, resultados e relatório.

## Estado

Os arquivos reais foram revisados e a primeira defesa foi concluída. O aluno
demonstrou boa compreensão dos dois gargalos, mas precisa tornar mais precisas as
afirmações sobre escalabilidade, competição e multithreading de hardware.

## Pontos que precisarão ser verificados

- Diferença entre programas limitados por largura de banda de memória e por CPU.
- Escalabilidade ao variar a quantidade de threads.
- Efeito do multithreading de hardware em cada tipo de carga.
- Estabilização ou piora de desempenho por competição de recursos.
- Resultados reais de `resultados_memory.txt` e `resultados_cpu.txt`.

## Próximo passo

Revisar os valores de speedup e eficiência, diferenciar estabilização de piora e
explicar por que a ausência de afinidade impede isolar o efeito do Hyper-Threading.

## Avaliações

### Defesa de 15/09/2026

**Nota: 8,0/10.**

Acertos demonstrados:

- explicou a soma `c[i] = a[i] + b[i]` e o tráfego útil de 24 bytes;
- definiu intensidade aritmética como operações por byte movimentado;
- relacionou baixa intensidade ao limite de largura de banda;
- explicou a reutilização de dados e a alta carga aritmética do compute-bound;
- reconheceu a saturação e os retornos decrescentes ao acrescentar threads;
- compreendeu a possibilidade de competição entre threads lógicas.

Erros ou lacunas observados:

- classificou como quase linear a escalabilidade de memória até quatro threads,
  embora o speedup com quatro tenha sido 2,026×;
- tratou 24 bytes como tráfego total, quando é o tráfego útil mínimo;
- usou expressões vagas como “existir recurso” e “recursos diminuindo”;
- inicialmente sugeriu piora contínua no teste de memória, mas houve melhora com
  ganhos marginais menores e estabilização perto de 50 GB/s;
- omitiu inicialmente números e características centrais do compute-bound;
- não apresentou de início a ressalva de que, sem afinidade, o experimento não
  isola Hyper-Threading, P-cores, E-cores e decisões do escalonador.

A defesa foi concluída antes da passagem para a Tarefa 6.
