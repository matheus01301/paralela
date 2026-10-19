#!/bin/bash
#SBATCH --job-name=afinidade
#SBATCH --partition=amd-512
#SBATCH --nodelist=r2n19
#SBATCH --time=0-0:50
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --mem=0
#SBATCH --cpus-per-task=128
#SBATCH --output=resultados.txt

# Mesmo no da Tarefa 12 (r2n19), como pede o enunciado.
cd "$SLURM_SUBMIT_DIR"
gcc -O2 -Wall -Wextra -std=c11 -fopenmp afinidade.c -o afinidade -lm

[ -f /etc/profile.d/modules.sh ] && . /etc/profile.d/modules.sh
module load libraries/numactl/numactl 2>/dev/null

echo "# no: $(hostname)"
gcc --version | head -1 | sed 's/^/# /'
echo "rotulo,threads,tempo"

# --- afinidade do OpenMP (slide 66), lugar = um nucleo fisico ---
export OMP_PLACES=cores
for politica in false true close spread master; do
    echo "## openmp_$politica"
    OMP_PROC_BIND=$politica ./afinidade "$politica" 128
done

# --- afinidade do sistema operacional ---
# taskset: o Linux so deixa o programa usar as CPUs 0-15 (um no NUMA).
echo "## so_taskset"
OMP_PROC_BIND=false taskset -c 0-15 ./afinidade taskset_0-15 16

# numactl --interleave: as paginas de memoria sao espalhadas por todos os nos NUMA.
echo "## so_numactl"
OMP_PROC_BIND=close numactl --interleave=all ./afinidade numactl_interleave 128

echo
echo "# topologia"
lscpu | grep -Ei "^NUMA node[0-9]" | sed 's/^/# /'
