/*
 * Tarefa 2 - Multiplicacao de matriz por vetor
 *
 * Compilar:
 *   gcc -O2 -Wall -Wextra -std=c99 mxv.c -o mxv.exe -lm
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* Impede que o compilador descarte chamadas repetidas durante a medicao. */
volatile double controle = 0.0;

double walltime(void)
{
    struct timespec tempo;
    clock_gettime(CLOCK_MONOTONIC, &tempo);
    return tempo.tv_sec + tempo.tv_nsec / 1000000000.0;
}

/* Versao 1: o laco interno percorre as colunas de uma mesma linha. */
void multiplicar_por_linhas(const double *matriz, const double *vetor,
                            double *resultado, int n)
{
    int i, j;

    for (i = 0; i < n; i++) {
        double soma = 0.0;

        for (j = 0; j < n; j++) {
            soma += matriz[(size_t)i * n + j] * vetor[j];
        }

        resultado[i] = soma;
    }
}

/* Versao 2: o laco interno percorre as linhas de uma mesma coluna. */
void multiplicar_por_colunas(const double *matriz, const double *vetor,
                             double *resultado, int n)
{
    int i, j;

    for (i = 0; i < n; i++) {
        resultado[i] = 0.0;
    }

    for (j = 0; j < n; j++) {
        for (i = 0; i < n; i++) {
            resultado[i] += matriz[(size_t)i * n + j] * vetor[j];
        }
    }
}

/* Repete os testes pequenos para que o tempo possa ser medido com estabilidade. */
int calcular_repeticoes(int n)
{
    long long elementos = (long long)n * n;
    int repeticoes = (int)(100000000LL / elementos);

    if (repeticoes < 1) {
        repeticoes = 1;
    }
    if (repeticoes > 10000) {
        repeticoes = 10000;
    }

    return repeticoes;
}

int main(void)
{
    int tamanhos[] = {64, 128, 256, 512, 1024, 2048, 4096};
    int quantidade = 7;
    int teste;

    printf("%-6s %-12s %-7s %-12s %-12s %s\n",
           "N", "Matriz (MB)", "Repet.", "Linhas (s)",
           "Colunas (s)", "Razao");

    for (teste = 0; teste < quantidade; teste++) {
        int n = tamanhos[teste];
        size_t elementos = (size_t)n * n;
        int repeticoes = calcular_repeticoes(n);
        double *matriz = malloc(elementos * sizeof(double));
        double *vetor = malloc((size_t)n * sizeof(double));
        double *resultado_linhas = malloc((size_t)n * sizeof(double));
        double *resultado_colunas = malloc((size_t)n * sizeof(double));
        double inicio, tempo_linhas, tempo_colunas;
        double maior_diferenca = 0.0;
        int i, r;

        if (matriz == NULL || vetor == NULL || resultado_linhas == NULL ||
            resultado_colunas == NULL) {
            printf("Nao foi possivel alocar memoria para N = %d.\n", n);
            free(matriz);
            free(vetor);
            free(resultado_linhas);
            free(resultado_colunas);
            return 1;
        }

        for (i = 0; i < (int)elementos; i++) {
            matriz[i] = (i % 10 + 1) * 0.1;
        }
        for (i = 0; i < n; i++) {
            vetor[i] = 1.0;
        }

        /* Aquecimento: executa uma vez antes de iniciar o cronometro. */
        multiplicar_por_linhas(matriz, vetor, resultado_linhas, n);
        multiplicar_por_colunas(matriz, vetor, resultado_colunas, n);

        inicio = walltime();
        for (r = 0; r < repeticoes; r++) {
            multiplicar_por_linhas(matriz, vetor, resultado_linhas, n);
            controle += resultado_linhas[r % n];
        }
        tempo_linhas = (walltime() - inicio) / repeticoes;

        inicio = walltime();
        for (r = 0; r < repeticoes; r++) {
            multiplicar_por_colunas(matriz, vetor, resultado_colunas, n);
            controle += resultado_colunas[r % n];
        }
        tempo_colunas = (walltime() - inicio) / repeticoes;

        for (i = 0; i < n; i++) {
            double diferenca = fabs(resultado_linhas[i] - resultado_colunas[i]);
            if (diferenca > maior_diferenca) {
                maior_diferenca = diferenca;
            }
        }

        printf("%-6d %-12.2f %-7d %-12.6f %-12.6f %.2fx%s\n",
               n, elementos * sizeof(double) / (1024.0 * 1024.0),
               repeticoes, tempo_linhas, tempo_colunas,
               tempo_colunas / tempo_linhas,
               maior_diferenca < 1e-9 ? "" : "  RESULTADOS DIFERENTES");

        free(matriz);
        free(vetor);
        free(resultado_linhas);
        free(resultado_colunas);
    }

    return 0;
}
