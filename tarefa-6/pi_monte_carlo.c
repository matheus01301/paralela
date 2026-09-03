/*
 * Tarefa 6 - Estimativa estocastica de pi com OpenMP
 *
 * Compilar:
 *   gcc -O2 -Wall -Wextra -std=c99 -fopenmp pi_monte_carlo.c -o pi_monte_carlo.exe
 *
 * Executar:
 *   ./pi_monte_carlo.exe [amostras] [threads] [semente]
 */

#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Gera numeros pseudoaleatorios sem usar um estado global compartilhado. */
static uint64_t misturar(uint64_t valor)
{
    valor = (valor ^ (valor >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    valor = (valor ^ (valor >> 27)) * UINT64_C(0x94d049bb133111eb);
    return valor ^ (valor >> 31);
}

static double uniforme(uint64_t valor)
{
    return (double)(misturar(valor) >> 11) / 9007199254740992.0;
}

/* O ponto depende apenas da semente e do indice da amostra. */
static void gerar_ponto(uint64_t semente, long long i, double *x, double *y)
{
    uint64_t base = semente + 2 * (uint64_t)i;
    *x = 2.0 * uniforme(base) - 1.0;
    *y = 2.0 * uniforme(base + 1) - 1.0;
}

static long long contar_sequencial(long long amostras, uint64_t semente)
{
    long long dentro = 0;

    for (long long i = 0; i < amostras; i++) {
        double x;
        double y;
        gerar_ponto(semente, i, &x, &y);
        if (x * x + y * y <= 1.0) {
            dentro++;
        }
    }
    return dentro;
}

/* Incorreta: todas as threads alteram o mesmo contador sem protecao. */
static long long contar_com_corrida(long long amostras, uint64_t semente)
{
    long long dentro = 0;

    #pragma omp parallel for
    for (long long i = 0; i < amostras; i++) {
        double x;
        double y;
        gerar_ponto(semente, i, &x, &y);
        if (x * x + y * y <= 1.0) {
            dentro++;
        }
    }
    return dentro;
}

/* Correta, mas cara: usa a regiao critica em cada ponto aceito. */
static long long contar_critical_por_ponto(long long amostras,
                                           uint64_t semente)
{
    long long dentro = 0;

    #pragma omp parallel for default(none) \
        shared(amostras, dentro) firstprivate(semente)
    for (long long i = 0; i < amostras; i++) {
        double x;
        double y;
        gerar_ponto(semente, i, &x, &y);
        if (x * x + y * y <= 1.0) {
            #pragma omp critical
            {
                dentro++;
            }
        }
    }
    return dentro;
}

/* Correta e eficiente: cada thread acumula antes de usar critical. */
static long long contar_critical_local(long long amostras, uint64_t semente,
                                       double *ultimo_x, double *ultimo_y)
{
    long long dentro = 0;
    long long dentro_local;
    long long i;
    double x;
    double y;
    double final_x = 0.0;
    double final_y = 0.0;

    #pragma omp parallel default(none) \
        shared(amostras, dentro, final_x, final_y) \
        firstprivate(semente) private(i, x, y, dentro_local)
    {
        dentro_local = 0;

        #pragma omp for lastprivate(final_x, final_y)
        for (i = 0; i < amostras; i++) {
            gerar_ponto(semente, i, &x, &y);
            final_x = x;
            final_y = y;
            if (x * x + y * y <= 1.0) {
                dentro_local++;
            }
        }

        #pragma omp critical
        {
            dentro += dentro_local;
        }
    }

    *ultimo_x = final_x;
    *ultimo_y = final_y;
    return dentro;
}

static void imprimir(const char *nome, long long dentro, long long amostras,
                     double tempo, long long referencia)
{
    double pi = 4.0 * (double)dentro / (double)amostras;
    printf("%-25s %12lld   %.9f   %10.6f   %+lld\n",
           nome, dentro, pi, tempo, dentro - referencia);
}

int main(int argc, char **argv)
{
    long long amostras = argc > 1 ? atoll(argv[1]) : 10000000;
    int threads = argc > 2 ? atoi(argv[2]) : omp_get_num_procs();
    uint64_t semente = argc > 3 ? strtoull(argv[3], NULL, 10) : 2026;
    long long sequencial;
    long long corrida;
    long long critico_por_ponto;
    long long critico_local;
    double tempo_sequencial;
    double tempo_corrida;
    double tempo_critico_por_ponto;
    double tempo_critico_local;
    double ultimo_x;
    double ultimo_y;
    double esperado_x;
    double esperado_y;
    double inicio;

    if (argc > 4 || amostras <= 0 || threads <= 0) {
        fprintf(stderr, "Uso: %s [amostras] [threads] [semente]\n", argv[0]);
        return EXIT_FAILURE;
    }

    omp_set_dynamic(0);
    omp_set_num_threads(threads);

    inicio = omp_get_wtime();
    sequencial = contar_sequencial(amostras, semente);
    tempo_sequencial = omp_get_wtime() - inicio;

    inicio = omp_get_wtime();
    corrida = contar_com_corrida(amostras, semente);
    tempo_corrida = omp_get_wtime() - inicio;

    inicio = omp_get_wtime();
    critico_por_ponto = contar_critical_por_ponto(amostras, semente);
    tempo_critico_por_ponto = omp_get_wtime() - inicio;

    inicio = omp_get_wtime();
    critico_local = contar_critical_local(amostras, semente,
                                          &ultimo_x, &ultimo_y);
    tempo_critico_local = omp_get_wtime() - inicio;

    gerar_ponto(semente, amostras - 1, &esperado_x, &esperado_y);

    printf("Estimativa estocastica de pi\n");
    printf("Amostras: %lld | Threads: %d | Semente: %llu\n\n",
           amostras, threads, (unsigned long long)semente);
    printf("Versao                         Dentro          pi    Tempo (s)    Diferenca\n");
    imprimir("Sequencial", sequencial, amostras, tempo_sequencial, sequencial);
    imprimir("parallel for (corrida)", corrida, amostras, tempo_corrida,
             sequencial);
    imprimir("critical por ponto", critico_por_ponto, amostras,
             tempo_critico_por_ponto, sequencial);
    imprimir("private + critical", critico_local, amostras,
             tempo_critico_local, sequencial);

    printf("\nUltimo ponto via lastprivate: (%.9f, %.9f)\n", ultimo_x, ultimo_y);
    printf("Ultimo ponto de referencia:   (%.9f, %.9f)\n", esperado_x, esperado_y);
    printf("Semente original: %llu\n", (unsigned long long)semente);

    return EXIT_SUCCESS;
}
