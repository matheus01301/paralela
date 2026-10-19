# Guia de apresentação — Tarefa 16

## Resumo em 30 segundos

y = A·x com MPI. `Scatter` divide A por linhas, `Bcast` manda x inteiro, cada
processo calcula as suas linhas e `Gather` junta y no processo 0. Medi cada fase: o
cálculo fica quase P vezes mais rápido, mas **distribuir A custa mais que calcular**,
então o total quase não melhora. Melhor caso: 2,43× com 16 processos em 8192².

## Números de cor

| | |
|---|---|
| cálculo com 8 processos | 7,4 a 7,8× mais rápido |
| distribuir / total | 52% (1 processo) a 97% (2048², 8 processos) |
| 2048², 1 processo | distribuir 4,17 ms > calcular 3,45 ms |
| melhor speedup total | 2,43× (8192², 16 processos) |
| 2048² | nunca ficou mais rápido que 1 processo |

## Se perguntarem

**O que é comunicação coletiva?** Comunicação que envolve todos os processos do
comunicador. Todos chamam a mesma função, na mesma ordem.

**Diferença entre Scatter e Bcast?** Scatter divide: cada processo recebe uma parte
diferente. Bcast copia: todos recebem o mesmo dado inteiro.

**Por que dividir por linhas?** Cada linha gera um elemento de y, sem depender das
outras. E a matriz está guardada linha por linha, então as linhas de cada processo
estão em sequência na memória: o Scatter só corta o vetor em pedaços iguais.

**Por que x vai inteiro?** Toda linha precisa de todos os elementos de x.

**Por que M tem que ser múltiplo de P?** O Scatter e o Gather mandam a mesma
quantidade para cada processo.

**Para que o `MPI_Barrier`?** Todos começam a medição juntos.

**Por que mais processos não aceleraram?** Razão computação/comunicação: cada número
de A é usado numa conta só (multiplica e soma), mas precisa ser enviado (8 bytes).
Mandar custa mais que calcular. E quem manda é só o processo 0, então essa parte não
diminui: funciona como a parte serial da lei de Amdahl.

**Como você sabe que é a distribuição?** Medi cada fase com `MPI_Wtime`. O cálculo
cai quase P vezes; a distribuição fica entre 52% e 97% do tempo.

**Por que com 16 processos melhorou?** Não medi. Hipótese: o Open MPI troca o
algoritmo do Scatter por uma árvore, em que vários processos repassam partes ao mesmo
tempo.

**Por que o cálculo parou de cair de 8 para 16 em 8192²?** Provavelmente os 16
processos ficaram na mesma região de memória e dividiram a mesma banda, como na
Tarefa 12. Não verifiquei.

**Como melhoraria?** Cada processo gerar ou ler as suas próprias linhas. Aí só x
precisaria ser enviado.

**Por que o Gather é barato?** Ele move só M números (y), contra M × N do Scatter.

## Detalhes do código

**Por que quatro `MPI_Wtime` (t0 a t3)?** Para medir cada fase: t1 − t0 é distribuir,
t2 − t1 é calcular, t3 − t2 é juntar.

**`atoi(argv[1]) % processos != 0`?** Confere se M é múltiplo do número de processos.
Se não for, o programa avisa e sai.

**Por que `a` e `y` só existem no processo 0?** Só ele guarda a matriz inteira e o
resultado inteiro. Os outros têm só `a_local` e `y_local`. Nos outros processos `a`
fica `NULL`, e `free(NULL)` não faz nada.

**`--cpu-bind=cores` no script?** Prende cada processo num núcleo.

**`export OMPI_MCA_btl=^openib`?** Desliga um componente antigo do Open MPI que só
gerava aviso.

## Comandos

```bash
ssh npad
cd ~/paralela/tarefa-16
sbatch job_npad.sh

cd tarefa-16/relatorio
python graficos.py && python build_pdf.py
```
