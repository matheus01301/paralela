/*
 * Tarefa 5 - Contagem sequencial e paralela de numeros primos
 *
 * Compilar:
 *   gcc -O2 -Wall -Wextra -std=c99 -fopenmp primos.c -o primos.exe
 *
 * Executar (argumentos opcionais: limite e numero de threads):
 *   ./primos.exe
 *   ./primos.exe 10000000 8
 */

#define _POSIX_C_SOURCE 199309L

#include <errno.h>
#include <limits.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

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

static int eh_primo(int numero)
{
    int divisor;

    if (numero < 2) {
        return 0;
    }
    if (numero == 2) {
        return 1;
    }
    if (numero % 2 == 0) {
        return 0;
    }

    for (divisor = 3; divisor <= numero / divisor; divisor += 2) {
        if (numero % divisor == 0) {
            return 0;
        }
    }
    return 1;
}

static long long contar_sequencial(int n)
{
    long long quantidade = 0;
    int numero;

    for (numero = 2; numero <= n; numero++) {
        if (eh_primo(numero)) {
            quantidade++;
        }
    }
    return quantidade;
}

static long long contar_paralelo(int n)
{
    long long quantidade = 0;
    int numero;

    /*
     * O incremento continua compartilhado de proposito. A diretiva paraleliza
     * o laco sem mudar sua logica e permite observar uma condicao de corrida.
     */
    #pragma omp parallel for
    for (numero = 2; numero <= n; numero++) {
        if (eh_primo(numero)) {
            quantidade++;
        }
    }
    return quantidade;
}

static int ler_inteiro_positivo(const char *texto, const char *nome)
{
    char *fim;
    long valor;

    errno = 0;
    valor = strtol(texto, &fim, 10);
    if (errno != 0 || *texto == '\0' || *fim != '\0' ||
        valor <= 0 || valor > INT_MAX) {
        fprintf(stderr, "Valor invalido para %s: %s\n", nome, texto);
        exit(EXIT_FAILURE);
    }
    return (int)valor;
}

int main(int argc, char **argv)
{
    int n = 10000000;
    int threads;
    long long resultado_sequencial;
    long long resultado_paralelo;
    double inicio;
    double tempo_sequencial;
    double tempo_paralelo;

    if (argc > 3) {
        fprintf(stderr, "Uso: %s [limite] [threads]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argc >= 2) {
        n = ler_inteiro_positivo(argv[1], "limite");
    }

    threads = omp_get_num_procs();
    if (argc == 3) {
        threads = ler_inteiro_positivo(argv[2], "threads");
    }
    omp_set_dynamic(0);
    omp_set_num_threads(threads);

    inicio = walltime();
    resultado_sequencial = contar_sequencial(n);
    tempo_sequencial = walltime() - inicio;

    inicio = walltime();
    resultado_paralelo = contar_paralelo(n);
    tempo_paralelo = walltime() - inicio;

    printf("Contagem de primos entre 2 e %d\n", n);
    printf("Threads na versao paralela: %d\n\n", threads);
    printf("Versao       Primos       Tempo (s)\n");
    printf("Sequencial   %-12lld %.6f\n", resultado_sequencial,
           tempo_sequencial);
    printf("Paralela     %-12lld %.6f\n", resultado_paralelo,
           tempo_paralelo);
    printf("\nSpeedup: %.3fx\n", tempo_sequencial / tempo_paralelo);
    printf("Diferenca no resultado: %lld\n",
           resultado_sequencial - resultado_paralelo);

    return EXIT_SUCCESS;
}
