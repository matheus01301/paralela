# Tarefa 16 — produto matriz-vetor com MPI

`matriz_vetor_mpi.c` calcula `y = A·x`. O processo 0 cria a matriz e o vetor;
`MPI_Scatter` entrega blocos de linhas, `MPI_Bcast` entrega `x` a todos e
`MPI_Gather` reúne os resultados. `M` precisa ser múltiplo do número de processos.
O programa mede o tempo total e o de cada fase (distribuir, calcular, juntar).

## Executar

```bash
sbatch job_npad.sh          # no NPAD: teste + 3 tamanhos x 1..16 processos, 5 vezes

# direto
mpicc -O2 -Wall -Wextra -std=c11 matriz_vetor_mpi.c -o matriz_vetor_mpi
mpirun -np 4 ./matriz_vetor_mpi 2048 2048
```

Saída (CSV): `m,n,processos,total,distribuir,calcular,juntar,y0,yultimo`.

## Arquivos

| Arquivo | Conteúdo |
|---|---|
| `matriz_vetor_mpi.c` | o programa |
| `job_npad.sh` | script Slurm (1 nó exclusivo, até 16 processos) |
| `resultados.txt` | saída usada no relatório |
| `relatorio/graficos.py` | gera os gráficos de tempo por fase (mediana de 5) |
| `relatorio/relatorio.pdf` | relatório com gráficos e código realçado |
| `guia_apresentacao.md` | perguntas para a defesa oral |
