#!/bin/bash
#SBATCH --job-name=matriz_vetor_mpi
#SBATCH --partition=amd-512
#SBATCH --nodes=1
#SBATCH --ntasks=8
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --mem=0
#SBATCH --time=0-0:15
#SBATCH --output=resultados.txt

set -e
cd "$SLURM_SUBMIT_DIR"
mpicc -O2 -Wall -Wextra -std=c11 matriz_vetor_mpi.c -o matriz_vetor_mpi

echo "no=$(hostname)"
mpicc --version | head -1
mpirun --version | head -1
echo "Teste pequeno: M=5 N=3; esperado y[0]=6, y[4]=18"
for p in 1 2 8; do
    srun --mpi=pmix -n "$p" ./matriz_vetor_mpi 5 3
done

echo "Medicoes: cada configuracao executada 5 vezes"
for m in 2048 4096 8192; do
    for p in 1 2 4 8; do
        for repeticao in 1 2 3 4 5; do
            srun --mpi=pmix -n "$p" ./matriz_vetor_mpi "$m" "$m"
        done
    done
done
