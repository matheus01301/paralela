/* ============================================================================
 * overhead_test.c - Quanto custa criar uma thread em C, para comparar
 *
 * Mede o mesmo que a PARTE 1 do overhead_test.mts: o custo de colocar N linhas
 * de execucao para trabalhar. A diferenca esperada e' de ordens de grandeza,
 * e o motivo esta' no que uma thread NAO precisa fazer:
 *
 *   - nao cria heap novo      (usa o do processo)
 *   - nao cria GC novo        (C nao tem GC)
 *   - nao carrega runtime     (ja esta' carregado no processo)
 *   - nao compila codigo      (ja esta' compilado no binario)
 *   - nao aquece JIT          (nao existe JIT)
 *
 * Uma thread e' um contador de programa e uma pilha. Um worker e' quase um
 * processo.
 *
 * Compilar: gcc -O2 -Wall -Wextra -fopenmp overhead_test.c -o overhead_test.exe
 * ==========================================================================*/

#include <stdio.h>
#include <omp.h>

#define REGIOES 10000

int main(void)
{
    int p = omp_get_max_threads();
    double t0, t_primeira, t_demais;
    int i;
    volatile int sentinela = 0;

    /* Primeira regiao paralela do programa: aqui o runtime do OpenMP realmente
     * cria as threads do sistema operacional. */
    t0 = omp_get_wtime();
#pragma omp parallel num_threads(p)
    {
        sentinela = omp_get_thread_num();
    }
    t_primeira = (omp_get_wtime() - t0) * 1000.0;

    /* Regioes seguintes: o OpenMP NAO destroi as threads no fim de uma regiao
     * paralela - ele as devolve a um pool e reaproveita na proxima. Este e' o
     * custo de "acordar" o pool, nao o de criar threads.
     *
     * O equivalente em Node seria um pool de workers reaproveitados, e essa e'
     * a otimizacao numero 1 de qualquer aplicacao web seria. */
    t0 = omp_get_wtime();
    for (i = 0; i < REGIOES; i++) {
#pragma omp parallel num_threads(p)
        {
            sentinela = omp_get_thread_num();
        }
    }
    t_demais = (omp_get_wtime() - t0) * 1000.0 / REGIOES;

    printf("Threads: %d\n\n", p);
    printf("  primeira regiao paralela (cria as threads) : %8.3f ms\n", t_primeira);
    printf("  regioes seguintes (pool reaproveitado)     : %8.4f ms  (media de %d)\n",
           t_demais, REGIOES);
    printf("\n  Por thread na criacao: %.1f microssegundos\n",
           t_primeira * 1000.0 / p);

    /* Le a sentinela so' para o compilador nao poder descartar as regioes
     * paralelas por "codigo sem efeito" - e para -Wall nao reclamar dela. */
    return sentinela > p ? 1 : 0;
}
