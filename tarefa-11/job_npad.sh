#!/bin/bash
#SBATCH --job-name=difusao_viscosa
#SBATCH --partition=amd-512
#SBATCH --time=0-0:30
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --cpus-per-task=128
#SBATCH --output=resultados.txt

# --exclusive: nenhum outro job divide o no, entao os tempos sao comparaveis.
# Cada thread fixa em um nucleo, para que so o schedule mude entre as medicoes.
export OMP_PLACES=cores
export OMP_PROC_BIND=close

cd "$SLURM_SUBMIT_DIR"
gcc -O2 -Wall -Wextra -std=c11 -fopenmp difusao_viscosa.c -o difusao_viscosa -lm

echo "# no: $(hostname)"
gcc --version | head -1 | sed 's/^/# /'

for threads in 16 32 64 128; do
    echo
    echo "## $threads threads"
    ./difusao_viscosa 1024 200 "$threads"
done
