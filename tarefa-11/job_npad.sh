#!/bin/bash
#SBATCH --job-name=difusao_viscosa
#SBATCH --partition=amd-512
#SBATCH --time=0-0:20
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --cpus-per-task=64

# --exclusive reserva o no inteiro. Sem isso, outro job dividiria os mesmos
# nucleos e os tempos medidos nao seriam comparaveis entre as variantes.

# Fixa cada thread em um nucleo. Sem isso, a migracao de threads entre nucleos
# mistura-se ao efeito de schedule que queremos medir.
export OMP_PLACES=cores
export OMP_PROC_BIND=close

cd "$SLURM_SUBMIT_DIR"

gcc -O2 -Wall -Wextra -std=c11 -fopenmp difusao_viscosa.c -o difusao_viscosa -lm

echo "no: $(hostname)   cpus alocadas: $SLURM_CPUS_PER_TASK"
gcc --version | head -1
echo

for threads in 16 32 64; do
    echo "==================== $threads threads ===================="
    ./difusao_viscosa 1024 200 "$threads" 5
    echo
done
