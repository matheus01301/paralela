#!/bin/bash
#SBATCH --job-name=topologia
#SBATCH --partition=amd-512
#SBATCH --time=0-0:5
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --cpus-per-task=128

export OMP_PLACES=cores
export OMP_PROC_BIND=close

echo "=== modelo e caches ==="
lscpu | grep -Ei "model name|^cpu\(s\)|core|socket|thread|numa|l1d|l2|l3"

echo
echo "=== numa ==="
numactl --hardware 2>/dev/null | head -30 || echo "numactl indisponivel"

echo
echo "=== cpus que o slurm deu ==="
echo "SLURM_CPUS_PER_TASK=$SLURM_CPUS_PER_TASK"
taskset -cp $$ 2>/dev/null
nproc

echo
echo "=== places do openmp ==="
cat > /tmp/places_$$.c <<'EOF'
#include <omp.h>
#include <stdio.h>
int main(void)
{
    printf("omp_get_num_places   = %d\n", omp_get_num_places());
    printf("omp_get_max_threads  = %d\n", omp_get_max_threads());
    for (int p = 0; p < omp_get_num_places() && p < 4; p++) {
        int n = omp_get_place_num_procs(p);
        int procs[64];
        omp_get_place_proc_ids(p, procs);
        printf("place %d tem %d proc(s):", p, n);
        for (int i = 0; i < n && i < 8; i++) printf(" %d", procs[i]);
        printf("\n");
    }
    return 0;
}
EOF
gcc -fopenmp /tmp/places_$$.c -o /tmp/places_$$ && /tmp/places_$$
rm -f /tmp/places_$$ /tmp/places_$$.c
