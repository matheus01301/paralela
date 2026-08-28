/*
 * Tarefa 4 - Benchmark limitado pela capacidade de calculo da CPU
 *
 * Compilar:
 *   gcc -O2 -Wall -Wextra -std=c99 -fopenmp cpu_bound.c -o cpu_bound.exe -lm
 *
 * Executar (argumentos opcionais: elementos e iteracoes internas):
 *   ./cpu_bound.exe
 *   ./cpu_bound.exe 1000000 200
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

/*
 * Quatro cadeias de dependencias independentes mantem as unidades aritmeticas
 * ocupadas. Ha muitas operacoes para apenas uma leitura e uma escrita.
 */
static void calcular(const double *entrada, double *saida, size_t n,
                     int iteracoes)
{
    long long i;

    #pragma omp parallel for schedule(static)
    for (i = 0; i < (long long)n; i++) {
        double x1 = entrada[i] + 0.101;
        double x2 = entrada[i] + 0.202;
        double x3 = entrada[i] + 0.303;
        double x4 = entrada[i] + 0.404;
        int k;

        for (k = 0; k < iteracoes; k++) {
            x1 = x1 * 0.99999991 + x2 * 0.00000017 + 0.00000011;
            x2 = x2 * 0.99999989 + x3 * 0.00000019 + 0.00000013;
            x3 = x3 * 0.99999987 + x4 * 0.00000023 + 0.00000017;
            x4 = x4 * 0.99999983 + x1 * 0.00000029 + 0.00000019;
        }
        saida[i] = x1 + x2 + x3 + x4;
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
    size_t n = 1000000;
    int iteracoes = 200;
    int max_threads;
    int lista_threads[20];
    int quantidade_threads;
    double *entrada;
    double *saida;
    double tempo_base = 0.0;
    size_t i;
    int teste;

    if (argc > 3) {
        fprintf(stderr, "Uso: %s [elementos] [iteracoes]\n", argv[0]);
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
        long long valor = ler_positivo(argv[2], "iteracoes");
        if (valor > INT_MAX) {
            fprintf(stderr, "Numero de iteracoes grande demais.\n");
            return EXIT_FAILURE;
        }
        iteracoes = (int)valor;
    }

    entrada = malloc(n * sizeof(*entrada));
    saida = malloc(n * sizeof(*saida));
    if (entrada == NULL || saida == NULL) {
        fprintf(stderr, "Nao foi possivel alocar %.1f MiB.\n",
                2.0 * n * sizeof(double) / (1024.0 * 1024.0));
        free(entrada);
        free(saida);
        return EXIT_FAILURE;
    }
    for (i = 0; i < n; i++) {
        entrada[i] = 0.5 + (double)(i % 1000) * 0.0001;
    }

    omp_set_dynamic(0);
    max_threads = omp_get_num_procs();
    quantidade_threads = montar_lista_threads(max_threads, lista_threads, 20);

    printf("Benchmark compute-bound: recorrencias aritmeticas independentes\n");
    printf("Elementos: %zu | iteracoes internas: %d | dados: %.1f MiB\n", n,
           iteracoes, 2.0 * n * sizeof(double) / (1024.0 * 1024.0));
    printf("Mediana de %d amostras\n", AMOSTRAS);
    printf("Intensidade aritmetica: %.1f FLOP/byte util\n\n",
           (double)iteracoes);
    printf("threads,tempo_s,speedup,eficiencia_pct,Giteracoes_s,GFLOP_s\n");

    for (teste = 0; teste < quantidade_threads; teste++) {
        int threads = lista_threads[teste];
        double tempos[AMOSTRAS];
        double mediana;
        double checksum = 0.0;
        int amostra;

        omp_set_num_threads(threads);
        calcular(entrada, saida, n, iteracoes); /* aquecimento */

        for (amostra = 0; amostra < AMOSTRAS; amostra++) {
            double inicio = walltime();
            calcular(entrada, saida, n, iteracoes);
            tempos[amostra] = walltime() - inicio;
        }

        qsort(tempos, AMOSTRAS, sizeof(tempos[0]), comparar_double);
        mediana = tempos[AMOSTRAS / 2];
        if (teste == 0) {
            tempo_base = mediana;
        }
        for (i = 0; i < n; i += 1024) {
            checksum += saida[i];
        }
        controle += checksum;

        printf("%d,%.9f,%.3f,%.1f,%.2f,%.2f\n", threads, mediana,
               tempo_base / mediana,
               100.0 * tempo_base / (mediana * threads),
               (double)n * iteracoes / mediana / 1.0e9,
               16.0 * n * iteracoes / mediana / 1.0e9);
    }

    if (controle == 0.123456789) {
        printf("controle: %.9f\n", controle);
    }
    free(entrada);
    free(saida);
    return EXIT_SUCCESS;
}
