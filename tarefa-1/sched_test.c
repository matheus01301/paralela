/* ============================================================================
 * sched_test.c - Por que a eficiencia cai? Desbalanceamento ou disputa?
 *
 * A eficiencia do pi_omp.c cai de 100% para ~40%. Duas hipoteses concorrentes:
 *
 *   H1 DESBALANCEAMENTO: o i7-13650HX tem 6 P-cores rapidos e 8 E-cores lentos.
 *      Com schedule(static) o OpenMP divide as iteracoes em fatias iguais, o
 *      P-core termina cedo e fica esperando o E-core. O tempo total e' o do
 *      nucleo mais lento.
 *      => se for isso, schedule(dynamic) resolve: quem termina pega mais.
 *
 *   H2 DISPUTA POR HARDWARE: sao 20 threads logicas em 14 nucleos fisicos. Duas
 *      threads irmas de hyperthreading COMPARTILHAM a unidade divisora de ponto
 *      flutuante. Como este laco e' limitado pelo divisor, a segunda thread nao
 *      tem por onde acelerar.
 *      => se for isso, NENHUM schedule resolve.
 *
 * Compilar: gcc -O2 -Wall -Wextra -fopenmp sched_test.c -o sched_test.exe -lm
 * ==========================================================================*/

#include <stdio.h>
#include <omp.h>

#define N 1000000000LL

/* Um macro por schedule, porque a clausula precisa ser escrita em tempo de
 * compilacao - nao da' para passar o tipo de escalonamento por variavel. */
#define RODA(NOME, CLAUSULA)                                    \
    static double NOME(int p)                                   \
    {                                                           \
        double soma = 0.0;                                      \
        long long k;                                            \
        _Pragma(CLAUSULA)                                       \
        for (k = 0; k < N; k++) {                               \
            double sinal = (k & 1) ? -1.0 : 1.0;                \
            soma += sinal / (double)(2 * k + 1);                \
        }                                                       \
        (void)p;                                                \
        return 4.0 * soma;                                      \
    }

RODA(estatico, "omp parallel for num_threads(p) reduction(+:soma) schedule(static)")
RODA(dinamico, "omp parallel for num_threads(p) reduction(+:soma) schedule(dynamic, 1000000)")
RODA(guiado,   "omp parallel for num_threads(p) reduction(+:soma) schedule(guided)")

int main(void)
{
    int ps[] = {6, 14, 20};   /* 6 P-cores | 14 nucleos fisicos | 20 logicas */
    int i;

    printf("N = %lld | comparando politicas de escalonamento\n\n", N);
    printf("  %8s  %12s  %12s  %12s\n", "THREADS", "static", "dynamic", "guided");
    printf("  --------------------------------------------------------\n");

    for (i = 0; i < 3; i++) {
        int p = ps[i];
        double t0, t_est, t_din, t_gui;

        t0 = omp_get_wtime(); estatico(p); t_est = omp_get_wtime() - t0;
        t0 = omp_get_wtime(); dinamico(p); t_din = omp_get_wtime() - t0;
        t0 = omp_get_wtime(); guiado(p);   t_gui = omp_get_wtime() - t0;

        printf("  %8d  %11.4fs  %11.4fs  %11.4fs\n", p, t_est, t_din, t_gui);
    }

    printf("\n  Se dynamic/guided forem bem mais rapidos que static, era\n");
    printf("  desbalanceamento (H1). Se os tres empatarem, e' disputa por\n");
    printf("  hardware (H2) e nenhum escalonamento salva.\n");

    return 0;
}
