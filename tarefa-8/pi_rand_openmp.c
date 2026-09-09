/*
 * Tarefa 8 - Monte Carlo, rand()/rand_r() e falso compartilhamento
 *
 * Linux/WSL:
 *   gcc -O2 -Wall -Wextra -std=c11 -fopenmp pi_rand_openmp.c -o pi_rand_openmp
 *   ./pi_rand_openmp [pontos] [threads] [semente]
 */

#define _POSIX_C_SOURCE 199506L

#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_THREADS 256
#define CACHE_LINE 64

/* Total solicitado para as versoes que usam critical. */
static long long total_global;

/* Posicoes distintas, mas intencionalmente contiguas na mesma linha de cache. */
_Alignas(CACHE_LINE) static volatile long long acertos_por_thread[MAX_THREADS];

/* rand_r nao existe na UCRT/MinGW. Esta implementacao de compatibilidade
 * permite compilar no Windows; no Linux, usa-se a funcao POSIX da libc. */
#ifdef _WIN32
static int rand_r(unsigned int *semente)
{
    unsigned int proximo = *semente;
    unsigned int resultado;

    proximo = proximo * 1103515245u + 12345u;
    resultado = (proximo / 65536u) % 2048u;
    proximo = proximo * 1103515245u + 12345u;
    resultado = (resultado << 10) ^ ((proximo / 65536u) % 1024u);
    proximo = proximo * 1103515245u + 12345u;
    resultado = (resultado << 10) ^ ((proximo / 65536u) % 1024u);
    *semente = proximo;
    return (int)(resultado % ((unsigned int)RAND_MAX + 1u));
}
#endif

typedef struct {
    long long acertos;
    double segundos;
} Resultado;

static double coordenada(int aleatorio)
{
    return (double)aleatorio / (double)RAND_MAX;
}

/* 1) Contador privado por thread e uma atualizacao global sob critical. */
static Resultado rand_critical(long long pontos, unsigned int semente)
{
    Resultado r;
    double inicio;

    total_global = 0;
    srand(semente);
    inicio = omp_get_wtime();

    #pragma omp parallel default(none) shared(pontos, total_global)
    {
        long long acertos_locais = 0;

        #pragma omp for schedule(static)
        for (long long i = 0; i < pontos; i++) {
            double x = coordenada(rand());
            double y = coordenada(rand());
            acertos_locais += (x * x + y * y <= 1.0);
        }

        #pragma omp critical
        total_global += acertos_locais;
    }

    r.segundos = omp_get_wtime() - inicio;
    r.acertos = total_global;
    return r;
}

/* 2) Cada thread incrementa diretamente sua posicao do vetor compartilhado.
 * volatile preserva os acessos a memoria que evidenciam falso compartilhamento. */
static Resultado rand_vetor(long long pontos, unsigned int semente,
                            int threads)
{
    Resultado r = {0, 0.0};
    double inicio;

    for (int i = 0; i < threads; i++) {
        acertos_por_thread[i] = 0;
    }
    srand(semente);
    inicio = omp_get_wtime();

    #pragma omp parallel default(none) shared(pontos, acertos_por_thread)
    {
        int id = omp_get_thread_num();

        #pragma omp for schedule(static)
        for (long long i = 0; i < pontos; i++) {
            double x = coordenada(rand());
            double y = coordenada(rand());
            if (x * x + y * y <= 1.0) {
                acertos_por_thread[id]++;
            }
        }
    }

    for (int i = 0; i < threads; i++) {
        r.acertos += acertos_por_thread[i];
    }
    r.segundos = omp_get_wtime() - inicio;
    return r;
}

/* 3) Igual a (1), mas cada thread possui o estado passado a rand_r. */
static Resultado rand_r_critical(long long pontos, unsigned int semente)
{
    Resultado r;
    double inicio;

    total_global = 0;
    inicio = omp_get_wtime();

    #pragma omp parallel default(none) shared(pontos, semente, total_global)
    {
        int id = omp_get_thread_num();
        unsigned int estado = semente + 0x9e3779b9u * (unsigned int)(id + 1);
        long long acertos_locais = 0;

        #pragma omp for schedule(static)
        for (long long i = 0; i < pontos; i++) {
            double x = coordenada(rand_r(&estado));
            double y = coordenada(rand_r(&estado));
            acertos_locais += (x * x + y * y <= 1.0);
        }

        #pragma omp critical
        total_global += acertos_locais;
    }

    r.segundos = omp_get_wtime() - inicio;
    r.acertos = total_global;
    return r;
}

/* 4) Igual a (2), agora com um estado rand_r privado por thread. */
static Resultado rand_r_vetor(long long pontos, unsigned int semente,
                              int threads)
{
    Resultado r = {0, 0.0};
    double inicio;

    for (int i = 0; i < threads; i++) {
        acertos_por_thread[i] = 0;
    }
    inicio = omp_get_wtime();

    #pragma omp parallel default(none) \
        shared(pontos, semente, acertos_por_thread)
    {
        int id = omp_get_thread_num();
        unsigned int estado = semente + 0x9e3779b9u * (unsigned int)(id + 1);

        #pragma omp for schedule(static)
        for (long long i = 0; i < pontos; i++) {
            double x = coordenada(rand_r(&estado));
            double y = coordenada(rand_r(&estado));
            if (x * x + y * y <= 1.0) {
                acertos_por_thread[id]++;
            }
        }
    }

    for (int i = 0; i < threads; i++) {
        r.acertos += acertos_por_thread[i];
    }
    r.segundos = omp_get_wtime() - inicio;
    return r;
}

static void imprimir(const char *nome, Resultado r, long long pontos)
{
    double pi = 4.0 * (double)r.acertos / (double)pontos;
    printf("%-18s %12lld   %.9f   %10.6f\n",
           nome, r.acertos, pi, r.segundos);
}

int main(int argc, char **argv)
{
    long long pontos = argc > 1 ? atoll(argv[1]) : 10000000;
    int threads = argc > 2 ? atoi(argv[2]) : omp_get_num_procs();
    unsigned int semente = argc > 3 ? (unsigned int)strtoul(argv[3], NULL, 10)
                                    : 2026u;
    Resultado resultados[4];

    if (argc > 4 || pontos <= 0 || threads <= 0 || threads > MAX_THREADS) {
        fprintf(stderr, "Uso: %s [pontos] [threads (1..%d)] [semente]\n",
                argv[0], MAX_THREADS);
        return EXIT_FAILURE;
    }

    omp_set_dynamic(0);
    omp_set_num_threads(threads);

    resultados[0] = rand_critical(pontos, semente);
    resultados[1] = rand_vetor(pontos, semente, threads);
    resultados[2] = rand_r_critical(pontos, semente);
    resultados[3] = rand_r_vetor(pontos, semente, threads);

    printf("Monte Carlo para pi\n");
    printf("Pontos: %lld | Threads: %d | Semente: %u\n\n",
           pontos, threads, semente);
    printf("Versao                  Acertos          pi    Tempo (s)\n");
    imprimir("rand + critical", resultados[0], pontos);
    imprimir("rand + vetor", resultados[1], pontos);
    imprimir("rand_r + critical", resultados[2], pontos);
    imprimir("rand_r + vetor", resultados[3], pontos);

    return EXIT_SUCCESS;
}
