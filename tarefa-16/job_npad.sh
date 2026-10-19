#!/bin/bash
#SBATCH --job-name=matriz_vetor_mpi
#SBATCH --partition=amd-512
#SBATCH --nodes=1
#SBATCH --ntasks=16
#SBATCH --exclusive
#SBATCH --mem=0
#SBATCH --time=0-0:20
#SBATCH --output=resultados.txt

set -e
cd "$SLURM_SUBMIT_DIR"
mpicc -O2 -Wall -Wextra -std=c11 matriz_vetor_mpi.c -o matriz_vetor_mpi

# A rede e usada pelo UCX; o componente antigo openib so gera um aviso.
export OMPI_MCA_btl=^openib

echo "# no: $(hostname)"
mpicc --version | head -1 | sed 's/^/# /'
mpirun --version | head -1 | sed 's/^/# /'
echo "# cada processo fixo em um core"
echo "m,n,processos,total,distribuir,calcular,juntar,y0,yultimo"

echo "# teste: M=8 N=3, esperado y0=6 e yultimo=27"
for p in 1 2 4 8; do
    srun --mpi=pmix -n "$p" --cpu-bind=cores ./matriz_vetor_mpi 8 3
done

echo "# medicoes: cada configuracao executada 5 vezes"
for m in 2048 4096 8192; do
    for p in 1 2 4 8 16; do
        for repeticao in 1 2 3 4 5; do
            srun --mpi=pmix -n "$p" --cpu-bind=cores ./matriz_vetor_mpi "$m" "$m"
        done
    done
done
