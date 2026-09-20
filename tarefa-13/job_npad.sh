#!/bin/bash
#SBATCH --job-name=afinidade
#SBATCH --partition=amd-512
#SBATCH --nodelist=r2n19
#SBATCH --time=0-0:50
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --mem=0
#SBATCH --cpus-per-task=128

cd "$SLURM_SUBMIT_DIR"

gcc -O2 -Wall -Wextra -std=c11 -fopenmp afinidade.c -o afinidade -lm

N=2048
PASSOS=200
EXEC=5
LISTA=1,4,8,16,32,64,128

echo "# no: $(hostname)"
gcc --version | head -1 | sed 's/^/# /'

[ -f /etc/profile.d/modules.sh ] && . /etc/profile.d/modules.sh
module load libraries/numactl/numactl 2>/dev/null
if command -v numactl > /dev/null; then
    NUMACTL=numactl
    echo "# numactl: $(command -v numactl)"
else
    NUMACTL=""
    echo "# numactl indisponivel no no de computacao"
fi

# Base unica de comparacao: um unico T(1), usado por todas as configuracoes.
# Sem isso, cada configuracao calcularia speedup contra o proprio T(1) e a
# eficiencia nao seria comparavel entre elas.
BASE=$(env -u OMP_PLACES -u OMP_PROC_BIND OMP_PROC_BIND=close OMP_PLACES=cores \
       ./afinidade base $N $PASSOS 9 1 | cut -d, -f4)
export TEMPO_BASE=$BASE
echo "# tempo base com 1 thread: $BASE s (mediana de 9)"
echo "# n=$N passos=$PASSOS execucoes=$EXEC (mediana)"
echo "rotulo,threads,n,tempo,speedup,eficiencia,dominios_l3,nos_numa,cpus_distintas"

rodar() {
    rotulo=$1; shift
    env -u OMP_PLACES -u OMP_PROC_BIND -u GOMP_CPU_AFFINITY "$@" \
        ./afinidade "$rotulo" $N $PASSOS $EXEC $LISTA
}

# --- afinidade do OpenMP -------------------------------------------------
rodar sem_afinidade   OMP_PROC_BIND=false
rodar master_cores    OMP_PROC_BIND=master  OMP_PLACES=cores
rodar close_cores     OMP_PROC_BIND=close   OMP_PLACES=cores
rodar spread_cores    OMP_PROC_BIND=spread  OMP_PLACES=cores
rodar close_threads   OMP_PROC_BIND=close   OMP_PLACES=threads
rodar spread_threads  OMP_PROC_BIND=spread  OMP_PLACES=threads
rodar close_sockets   OMP_PROC_BIND=close   OMP_PLACES=sockets
rodar spread_sockets  OMP_PROC_BIND=spread  OMP_PLACES=sockets
rodar close_numa      OMP_PROC_BIND=close   OMP_PLACES=numa_domains

# --- afinidade do sistema operacional ------------------------------------
rodar so_gomp_lista   GOMP_CPU_AFFINITY=0-127
rodar so_gomp_espalha GOMP_CPU_AFFINITY=0-127:16

if [ -n "$NUMACTL" ]; then
    env -u OMP_PLACES -u OMP_PROC_BIND -u GOMP_CPU_AFFINITY \
        OMP_PROC_BIND=close OMP_PLACES=cores \
        $NUMACTL --interleave=all ./afinidade so_interleave $N $PASSOS $EXEC $LISTA
    env -u OMP_PLACES -u OMP_PROC_BIND -u GOMP_CPU_AFFINITY \
        OMP_PROC_BIND=close OMP_PLACES=cores \
        $NUMACTL --cpunodebind=0 --membind=0 ./afinidade so_numa0 $N $PASSOS $EXEC 1,4,8,16
fi

# --- mapas de colocacao --------------------------------------------------
echo
for cfg in close spread; do
    for lugar in cores threads sockets; do
        env -u OMP_PLACES -u OMP_PROC_BIND \
            OMP_PROC_BIND=$cfg OMP_PLACES=$lugar MAPA=1 \
            ./afinidade "${cfg}_${lugar}" $N 2 1 8 2>/dev/null | grep '^#'
    done
done

echo
echo "# topologia do no"
lscpu | grep -Ei "^NUMA node[0-9]" | sed 's/^/# /'
