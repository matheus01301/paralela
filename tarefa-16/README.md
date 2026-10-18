# Tarefa 16 — produto matriz-vetor com MPI

`matriz_vetor_mpi.c` calcula `y = A·x`. O processo 0 cria a matriz e o vetor;
`MPI_Scatter` entrega blocos de linhas, `MPI_Bcast` entrega `x` a todos e
`MPI_Gather` reúne os resultados. Quando `M` não é múltiplo do número de
processos, as últimas linhas distribuídas são zeros e não fazem parte de `y`.

## Executar

```bash
mpicc -O2 -Wall -Wextra -std=c11 matriz_vetor_mpi.c -o matriz_vetor_mpi
mpirun -np 4 ./matriz_vetor_mpi 2048 2048
```

Os argumentos são `M` (linhas) e `N` (colunas). O programa imprime o tempo em
segundos e o primeiro e o último elemento de `y`. Para repetir as medições no
NPAD, use `sbatch job_npad.sh` no diretório da tarefa.

O [relatório em PDF](relatorio/relatorio.pdf) contém resultados, explicações
curtas para a defesa e o código com sintaxe colorida. Ele é gerado com
`python relatorio/build_pdf.py` (dependências: `markdown-it-py`, `Pygments`
e Chrome ou Edge).
