/* ============================================================================
 * pi_omp.c - Aproximacao de PI com OpenMP: speedup, eficiencia e Karp-Flatt
 *
 * Varre de 1 ate omp_get_max_threads() threads, sempre com o mesmo N, e mede
 * quanto o programa acelera. Tambem demonstra a condicao de corrida que
 * aparece quando se esquece o reduction.
 *
 * Compilar:  gcc -O2 -Wall -Wextra -fopenmp pi_omp.c -o pi_omp.exe -lm
 * Executar:  ./pi_omp.exe            (N = 10^9)
 *            ./pi_omp.exe 100000000  (N menor, para testar rapido)
 * ==========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

static const double PI_REF = 3.14159265358979323846;

/* ---------------------------------------------------------------------------
 * A DEPENDENCIA CARREGADA PELO LACO (loop-carried dependency)
 *
 * No pi_serie.c o sinal alternava assim:
 *
 *     sinal = -sinal;          <-- o valor da iteracao k depende da k-1
 *
 * Isso e' uma dependencia carregada pelo laco, e ela IMPEDE a paralelizacao:
 * a thread que pegar a iteracao 500.000.000 nao tem como saber o sinal sem ter
 * executado as 499.999.999 anteriores.
 *
 * A solucao e' reescrever o sinal como FUNCAO DE k, sem olhar para o passado:
 *
 *     sinal = (k par) ? +1 : -1
 *
 * Aqui isso vira (k & 1), que testa o bit menos significativo - o jeito barato
 * de perguntar "k e' impar?". Cada iteracao passa a ser independente das outras,
 * e o laco fica paralelizavel.
 *
 * Reconhecer e eliminar esse tipo de dependencia e' metade do trabalho de
 * paralelizar codigo real.
 * -------------------------------------------------------------------------*/

/* Versao CORRETA: cada thread acumula em sua propria copia de soma, e o OpenMP
 * combina todas no final. E' isso que a clausula reduction(+:soma) faz. */
static double leibniz_reduction(long long n, int nthreads)
{
    double soma = 0.0;
    long long k;

#pragma omp parallel for num_threads(nthreads) reduction(+ : soma)
    for (k = 0; k < n; k++) {
        double sinal = (k & 1) ? -1.0 : 1.0;
        soma += sinal / (double)(2 * k + 1);
    }
    return 4.0 * soma;
}

/* Versao ERRADA: sem reduction, `soma` e' compartilhada por todas as threads.
 * O "soma += x" e' ler-somar-escrever, nao atomico. Duas threads que leem o
 * mesmo valor antes de qualquer uma escrever perdem um dos incrementos.
 * E' exatamente o caso B do memoria_demo.mts, mas aqui acontece SEM que voce
 * peca nada - basta esquecer uma clausula. */
static double leibniz_corrida(long long n, int nthreads)
{
    double soma = 0.0;
    long long k;

#pragma omp parallel for num_threads(nthreads)
    for (k = 0; k < n; k++) {
        double sinal = (k & 1) ? -1.0 : 1.0;
        soma += sinal / (double)(2 * k + 1); /* <-- corrida de dados */
    }
    return 4.0 * soma;
}

/* Versao com omp critical: correta, porem desastrosa em desempenho. Serve para
 * medir o custo de serializar uma secao critica dentro de um laco quente.
 * So roda com N reduzido, senao demora demais. */
static double leibniz_critical(long long n, int nthreads)
{
    double soma = 0.0;
    long long k;

#pragma omp parallel for num_threads(nthreads)
    for (k = 0; k < n; k++) {
        double sinal = (k & 1) ? -1.0 : 1.0;
        double termo = sinal / (double)(2 * k + 1);
#pragma omp critical
        soma += termo;
    }
    return 4.0 * soma;
}

int main(int argc, char *argv[])
{
    long long n = 1000000000LL; /* 10^9 */
    int max_threads = omp_get_max_threads();
    int t;
    double t_base = 0.0;

    if (argc > 1) {
        n = atoll(argv[1]);
        if (n < 1) {
            fprintf(stderr, "Uso: %s [N]\n", argv[0]);
            return 1;
        }
    }

    printf("OpenMP %d | nucleos visiveis: %d | N = %lld\n\n",
           _OPENMP, max_threads, n);

    /* ---- Demonstracao da condicao de corrida --------------------------- */
    printf("=== Esquecendo o reduction (3 execucoes, %d threads) ===\n", max_threads);
    for (t = 0; t < 3; t++) {
        double pi_errado = leibniz_corrida(n / 100, max_threads);
        printf("  pi = %.15f   erro = %.3e\n", pi_errado, fabs(PI_REF - pi_errado));
    }
    printf("  (valores diferentes a cada execucao: o resultado depende de quem\n");
    printf("   escreveu por ultimo. Compare com o correto: %.15f)\n\n", PI_REF);

    /* ---- Custo de uma secao critica ------------------------------------ */
    {
        double t0, t_crit, t_red;
        long long n_pequeno = n / 1000;

        t0 = omp_get_wtime();
        leibniz_critical(n_pequeno, max_threads);
        t_crit = omp_get_wtime() - t0;

        t0 = omp_get_wtime();
        leibniz_reduction(n_pequeno, max_threads);
        t_red = omp_get_wtime() - t0;

        printf("=== Custo da secao critica (N = %lld, %d threads) ===\n",
               n_pequeno, max_threads);
        printf("  omp critical : %8.4f s\n", t_crit);
        printf("  reduction    : %8.4f s\n", t_red);
        printf("  critical e' %.0fx mais lento - todas as threads enfileiram\n",
               t_red > 0 ? t_crit / t_red : 0.0);
        printf("  para entrar uma de cada vez na mesma linha de codigo.\n\n");
    }

    /* ---- Varredura de threads ------------------------------------------ */
    printf("=== Speedup com reduction (N = %lld) ===\n\n", n);
    printf("  %7s  %-17s %10s  %9s  %8s  %10s  %12s\n",
           "THREADS", "APROXIMACAO", "ERRO", "TEMPO(s)", "SPEEDUP", "EFICIENCIA",
           "KARP-FLATT");
    printf("  --------------------------------------------------------"
           "---------------------------------\n");

    for (t = 1; t <= max_threads; t++) {
        double t0, tempo, pi_val, speedup, efic, karp;

        t0 = omp_get_wtime();
        pi_val = leibniz_reduction(n, t);
        tempo = omp_get_wtime() - t0;

        if (t == 1) t_base = tempo;

        speedup = t_base / tempo;
        efic = speedup / (double)t;

        /* Metrica de Karp-Flatt: estima experimentalmente a fracao serial `e`
         * do programa a partir do speedup medido.
         *
         *     e = (1/S - 1/p) / (1 - 1/p)
         *
         * Se `e` fica constante conforme p cresce, o limite do speedup e' de
         * fato a parte serial do algoritmo (lei de Amdahl pura). Se `e` CRESCE
         * com p, o culpado nao e' o algoritmo: e' o overhead de paralelizacao
         * (criar threads, sincronizar, disputar cache). Diagnostico que o
         * speedup sozinho nao da'. */
        if (t == 1) {
            printf("  %7d  %.15f  %.3e  %9.4f  %8s  %10s  %12s\n",
                   t, pi_val, fabs(PI_REF - pi_val), tempo, "1.00", "100%", "-");
        } else {
            karp = (1.0 / speedup - 1.0 / t) / (1.0 - 1.0 / t);
            printf("  %7d  %.15f  %.3e  %9.4f  %8.2f  %9.0f%%  %12.4f\n",
                   t, pi_val, fabs(PI_REF - pi_val), tempo, speedup,
                   efic * 100.0, karp);
        }
    }

    printf("\n  Speedup    = tempo(1 thread) / tempo(p threads)\n");
    printf("  Eficiencia = speedup / p  (100%% = escalabilidade perfeita)\n");
    printf("  Karp-Flatt = fracao serial estimada; se cresce com p, o problema\n");
    printf("               e' overhead, nao a parte sequencial do algoritmo.\n");

    return 0;
}
