#!/bin/bash
#SBATCH --job-name=escalabilidade
#SBATCH --partition=amd-512
#SBATCH --time=0-0:40
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --mem=0
#SBATCH --cpus-per-task=128

export OMP_PLACES=cores
export OMP_PROC_BIND=close

cd "$SLURM_SUBMIT_DIR"

gcc -O2 -Wall -Wextra -std=c11 -fopenmp escalabilidade.c -o escalabilidade -lm

echo "# no: $(hostname)   cpus alocadas: $SLURM_CPUS_PER_TASK"
gcc --version | head -1 | sed 's/^/# /'

./escalabilidade verificar 256 40

echo
./escalabilidade forte 2048 200 5 128

echo
./escalabilidade fraca 1024 200 5 64
