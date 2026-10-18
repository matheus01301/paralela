# Tarefa 17 — produto matriz-vetor por colunas com MPI

`matriz_vetor_colunas_mpi.c` calcula `y = A·x` distribuindo colunas. O tipo
`MPI_Type_vector` descreve as colunas separadas por linhas da matriz;
`MPI_Type_create_resized` posiciona os blocos seguintes para o `MPI_Scatter`.
Outro `MPI_Scatter` reparte `x`. Cada processo calcula um vetor parcial de
tamanho `M`, e `MPI_Reduce` com `MPI_SUM` forma `y` no processo 0.

Quando `N` não é múltiplo do número de processos, as colunas extras são zeros.
Assim as duas chamadas de `Scatter` usam blocos de tamanho igual.

## Executar

```bash
mpicc -O2 -Wall -Wextra -std=c11 matriz_vetor_colunas_mpi.c -o matriz_vetor_colunas_mpi
mpirun -np 4 ./matriz_vetor_colunas_mpi 2048 2048
```

`M` é o número de linhas, `N` o de colunas. No NPAD, `sbatch job_npad.sh`
executa os testes e mede, no mesmo nó, esta versão e a da Tarefa 16. O script
usa `../tarefa-16/matriz_vetor_mpi.c`, portanto as duas pastas devem estar lado
a lado. A saída bruta do job é `resultados_brutos.txt`; a cópia usada no
relatório está em [resultados.txt](resultados.txt).

O [PDF](relatorio/relatorio.pdf) traz a comparação e o código colorido.
Para reconstruí-lo: `python relatorio/build_pdf.py` (requer `markdown-it-py`,
`Pygments` e Chrome ou Edge).
