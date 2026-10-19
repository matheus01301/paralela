#!/bin/bash
#SBATCH --job-name=pingpong
#SBATCH --partition=amd-512
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=2
#SBATCH --exclusive
#SBATCH --mem=0
#SBATCH --time=0-0:15
#SBATCH --output=resultados.txt

set -e
cd "$SLURM_SUBMIT_DIR"
mpicc -O2 -Wall -Wextra -std=c11 pingpong.c -o pingpong

# A rede e usada pelo UCX; o componente antigo openib so gera um aviso.
export OMPI_MCA_btl=^openib

echo "# nos alocados: $SLURM_JOB_NODELIST"
mpicc --version | head -1 | sed 's/^/# /'
mpirun --version | head -1 | sed 's/^/# /'
echo "# cada cenario executado 3 vezes; cada processo fixo em um core"

for repeticao in 1 2 3; do
    # Os dois processos no mesmo no: a mensagem passa pela memoria do no.
    echo "## mesmo_no"
    srun --mpi=pmix -N 1 -n 2 --cpu-bind=cores hostname | sed 's/^/# processo em: /'
    srun --mpi=pmix -N 1 -n 2 --cpu-bind=cores ./pingpong

    # Um processo em cada no: a mensagem passa pela rede do cluster.
    echo "## nos_diferentes"
    srun --mpi=pmix -N 2 -n 2 --ntasks-per-node=1 --cpu-bind=cores hostname | sed 's/^/# processo em: /'
    srun --mpi=pmix -N 2 -n 2 --ntasks-per-node=1 --cpu-bind=cores ./pingpong
done
