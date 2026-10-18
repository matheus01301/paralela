#!/bin/bash
#SBATCH --job-name=colunas_mpi
#SBATCH --partition=amd-512
#SBATCH --nodelist=r2n09
#SBATCH --nodes=1
#SBATCH --ntasks=8
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --mem=0
#SBATCH --time=0-0:20
#SBATCH --output=resultados_brutos.txt

set -euo pipefail
cd "$SLURM_SUBMIT_DIR"
export OMPI_MCA_btl=^openib

mpicc -O2 -Wall -Wextra -std=c11 matriz_vetor_colunas_mpi.c -o matriz_vetor_colunas_mpi
mpicc -O2 -Wall -Wextra -std=c11 ../tarefa-16/matriz_vetor_mpi.c -o matriz_vetor_linhas_mpi

echo "no=$(hostname)"
mpicc --version | sed -n '1p'
mpirun --version | sed -n '1p'
echo "Teste pequeno: M=5 N=3; esperado y[0]=6, y[4]=18"
for p in 1 2 8; do
    srun --mpi=pmix -n "$p" ./matriz_vetor_colunas_mpi 5 3 | sed 's/^M=/colunas M=/'
done
echo "Teste retangular: M=3 N=5; esperado y[0]=15, y[2]=25"
for p in 2 8; do
    srun --mpi=pmix -n "$p" ./matriz_vetor_colunas_mpi 3 5 | sed 's/^M=/colunas M=/'
done

echo "Medicoes: 5 repeticoes por configuracao e versao"
for m in 2048 4096 8192; do
    for p in 1 2 4 8; do
        for repeticao in 1 2 3 4 5; do
            srun --mpi=pmix -n "$p" ./matriz_vetor_linhas_mpi "$m" "$m" | sed 's/^M=/linhas M=/'
            srun --mpi=pmix -n "$p" ./matriz_vetor_colunas_mpi "$m" "$m" | sed 's/^M=/colunas M=/'
        done
    done
done
