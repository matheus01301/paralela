/*
 * Tarefa 4 - Benchmark limitado pela largura de banda da memoria
 *
 * Compilar:
 *   gcc -O2 -Wall -Wextra -std=c99 -fopenmp memory_bound.c -o memory_bound.exe
 *
 * Executar (argumentos opcionais: elementos e repeticoes por amostra):
 *   ./memory_bound.exe
 *   ./memory_bound.exe 33554432 4
 */

#define _POSIX_C_SOURCE 199309L

#include <errno.h>
#include <limits.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#define AMOSTRAS 5

static volatile double controle = 0.0;

static double walltime(void)
{
#ifdef _WIN32
    LARGE_INTEGER contador;
    LARGE_INTEGER frequencia;
    QueryPerformanceCounter(&contador);
    QueryPerformanceFrequency(&frequencia);
    return (double)contador.QuadPart / (double)frequencia.QuadPart;
#else
    struct timespec tempo;
    clock_gettime(CLOCK_MONOTONIC, &tempo);
    return tempo.tv_sec + tempo.tv_nsec / 1000000000.0;
#endif
}

static int comparar_double(const void *a, const void *b)
{
    double x = *(const double *)a;
    double y = *(const double *)b;
    return (x > y) - (x < y);
}

static long long ler_positivo(const char *texto, const char *nome)
{
    char *fim;
    long long valor;

    errno = 0;
    valor = strtoll(texto, &fim, 10);
    if (errno != 0 || *texto == '\0' || *fim != '\0' || valor <= 0) {
        fprintf(stderr, "Valor invalido para %s: %s\n", nome, texto);
        exit(EXIT_FAILURE);
    }
    return valor;
}

/* Cada iteracao le dois doubles e grava um: baixa intensidade aritmetica. */
static void somar_vetores(const double *a, const double *b, double *c,
                          size_t n)
{
    long long i;

    #pragma omp parallel for schedule(static)
    for (i = 0; i < (long long)n; i++) {
        c[i] = a[i] + b[i];
    }
}

static int montar_lista_threads(int maximo, int *lista, int capacidade)
{
    const int candidatos[] = {1, 2, 4, 6, 8, 10, 12, 14, 16, 20,
                              24, 32, 48, 64, 96, 128};
    const int quantidade = (int)(sizeof(candidatos) / sizeof(candidatos[0]));
    int usados = 0;
    int i;

    for (i = 0; i < quantidade && usados < capacidade; i++) {
        if (candidatos[i] <= maximo) {
            lista[usados++] = candidatos[i];
        }
    }
    if (usados < capacidade && lista[usados - 1] != maximo) {
        lista[usados++] = maximo;
    }
    return usados;
}

int main(int argc, char **argv)
{
    size_t n = 32ULL * 1024ULL * 1024ULL;
    int repeticoes = 4;
    int max_threads;
    int lista_threads[20];
    int quantidade_threads;
    double *a;
    double *b;
    double *c;
    double tempo_base = 0.0;
    size_t i;
    int teste;

    if (argc > 3) {
        fprintf(stderr, "Uso: %s [elementos] [repeticoes]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argc >= 2) {
        long long valor = ler_positivo(argv[1], "elementos");
        if ((unsigned long long)valor > SIZE_MAX / sizeof(double)) {
            fprintf(stderr, "Vetor grande demais.\n");
            return EXIT_FAILURE;
        }
        n = (size_t)valor;
    }
    if (argc == 3) {
        long long valor = ler_positivo(argv[2], "repeticoes");
        if (valor > INT_MAX) {
            fprintf(stderr, "Numero de repeticoes grande demais.\n");
            return EXIT_FAILURE;
        }
        repeticoes = (int)valor;
    }

    a = malloc(n * sizeof(*a));
    b = malloc(n * sizeof(*b));
    c = malloc(n * sizeof(*c));
    if (a == NULL || b == NULL || c == NULL) {
        fprintf(stderr, "Nao foi possivel alocar %.1f MiB.\n",
                3.0 * n * sizeof(double) / (1024.0 * 1024.0));
        free(a);
        free(b);
        free(c);
        return EXIT_FAILURE;
    }

    for (i = 0; i < n; i++) {
        a[i] = 1.0 + (double)(i % 97) * 0.001;
        b[i] = 2.0 - (double)(i % 89) * 0.001;
    }

    omp_set_dynamic(0);
    max_threads = omp_get_num_procs();
    quantidade_threads = montar_lista_threads(max_threads, lista_threads, 20);

    printf("Benchmark memory-bound: c[i] = a[i] + b[i]\n");
    printf("Elementos por vetor: %zu | memoria total: %.1f MiB\n", n,
           3.0 * n * sizeof(double) / (1024.0 * 1024.0));
    printf("Mediana de %d amostras, %d repeticoes por amostra\n",
           AMOSTRAS, repeticoes);
    printf("Intensidade aritmetica: %.6f FLOP/byte util\n\n", 1.0 / 24.0);
    printf("threads,tempo_s,speedup,eficiencia_pct,largura_GB_s,GFLOP_s\n");

    for (teste = 0; teste < quantidade_threads; teste++) {
        int threads = lista_threads[teste];
        double tempos[AMOSTRAS];
        double mediana;
        double checksum = 0.0;
        double bytes;
        int amostra;
        int r;

        omp_set_num_threads(threads);
        somar_vetores(a, b, c, n); /* aquecimento e primeira alocacao fisica */

        for (amostra = 0; amostra < AMOSTRAS; amostra++) {
            double inicio = walltime();
            for (r = 0; r < repeticoes; r++) {
                somar_vetores(a, b, c, n);
            }
            tempos[amostra] = (walltime() - inicio) / repeticoes;
        }

        qsort(tempos, AMOSTRAS, sizeof(tempos[0]), comparar_double);
        mediana = tempos[AMOSTRAS / 2];
        if (teste == 0) {
            tempo_base = mediana;
        }
        for (i = 0; i < n; i += 4096) {
            checksum += c[i];
        }
        controle += checksum;

        /* Trafego util minimo: duas leituras e uma escrita por elemento. */
        bytes = 3.0 * n * sizeof(double);
        printf("%d,%.9f,%.3f,%.1f,%.2f,%.3f\n", threads, mediana,
               tempo_base / mediana,
               100.0 * tempo_base / (mediana * threads),
               bytes / mediana / 1.0e9,
               (double)n / mediana / 1.0e9);
    }

    if (controle == 0.123456789) {
        printf("controle: %.9f\n", controle);
    }
    free(a);
    free(b);
    free(c);
    return EXIT_SUCCESS;
}
