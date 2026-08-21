/*
 * Tarefa 3 - Paralelismo ao nivel de instrucao (ILP)
 *
 * Compilar:
 *   gcc -O0 -Wall -Wextra -std=c99 ilp.c -o ilp_O0.exe
 *   gcc -O2 -Wall -Wextra -std=c99 ilp.c -o ilp_O2.exe
 *   gcc -O3 -Wall -Wextra -std=c99 ilp.c -o ilp_O3.exe
 *
 * Executar um teste por processo, evitando influencia entre as somas:
 *   ./ilp_O2.exe inicializacao
 *   ./ilp_O2.exe dependente
 *   ./ilp_O2.exe multipla2
 *   ./ilp_O2.exe multipla4
 *   ./ilp_O2.exe multipla8
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TAMANHO 16000000
#define TEMPO_ALVO 0.6

/* Torna o resultado observavel e impede a remocao dos testes pelo compilador. */
volatile double controle = 0.0;

double walltime(void)
{
    struct timespec tempo;
    clock_gettime(CLOCK_MONOTONIC, &tempo);
    return tempo.tv_sec + tempo.tv_nsec / 1000000000.0;
}

/* Laco 1: inicializacao independente. Cada iteracao escreve uma posicao. */
__attribute__((noinline))
void inicializar(double *vetor, int n, double ajuste)
{
    int i;

    for (i = 0; i < n; i++) {
        vetor[i] = i * 0.5 + 1.0 + ajuste;
    }
}

/* Laco 2: uma unica soma cria uma cadeia de dependencias. */
__attribute__((noinline))
double soma_dependente(const double *vetor, int n)
{
    double soma = 0.0;
    int i;

    for (i = 0; i < n; i++) {
        soma += vetor[i];
    }

    return soma;
}

/* Laco 3a: duas cadeias de soma independentes. */
__attribute__((noinline))
double soma_multipla2(const double *vetor, int n)
{
    double soma0 = 0.0;
    double soma1 = 0.0;
    int i;

    for (i = 0; i + 1 < n; i += 2) {
        soma0 += vetor[i];
        soma1 += vetor[i + 1];
    }
    for (; i < n; i++) {
        soma0 += vetor[i];
    }

    return soma0 + soma1;
}

/* Laco 3b: quatro cadeias de soma independentes. */
__attribute__((noinline))
double soma_multipla4(const double *vetor, int n)
{
    double soma0 = 0.0, soma1 = 0.0;
    double soma2 = 0.0, soma3 = 0.0;
    int i;

    for (i = 0; i + 3 < n; i += 4) {
        soma0 += vetor[i];
        soma1 += vetor[i + 1];
        soma2 += vetor[i + 2];
        soma3 += vetor[i + 3];
    }
    for (; i < n; i++) {
        soma0 += vetor[i];
    }

    return soma0 + soma1 + soma2 + soma3;
}

/* Laco 3c: oito cadeias de soma independentes. */
__attribute__((noinline))
double soma_multipla8(const double *vetor, int n)
{
    double soma0 = 0.0, soma1 = 0.0;
    double soma2 = 0.0, soma3 = 0.0;
    double soma4 = 0.0, soma5 = 0.0;
    double soma6 = 0.0, soma7 = 0.0;
    int i;

    for (i = 0; i + 7 < n; i += 8) {
        soma0 += vetor[i];
        soma1 += vetor[i + 1];
        soma2 += vetor[i + 2];
        soma3 += vetor[i + 3];
        soma4 += vetor[i + 4];
        soma5 += vetor[i + 5];
        soma6 += vetor[i + 6];
        soma7 += vetor[i + 7];
    }
    for (; i < n; i++) {
        soma0 += vetor[i];
    }

    return soma0 + soma1 + soma2 + soma3 +
           soma4 + soma5 + soma6 + soma7;
}

double executar_soma(const char *teste, const double *vetor, int n)
{
    if (strcmp(teste, "dependente") == 0) {
        return soma_dependente(vetor, n);
    }
    if (strcmp(teste, "multipla2") == 0) {
        return soma_multipla2(vetor, n);
    }
    if (strcmp(teste, "multipla4") == 0) {
        return soma_multipla4(vetor, n);
    }
    return soma_multipla8(vetor, n);
}

void medir_inicializacao(double *vetor, int n)
{
    int repeticoes = 1;
    double tempo_total;
    int r;

    do {
        double inicio = walltime();

        for (r = 0; r < repeticoes; r++) {
            inicializar(vetor, n, r * 0.001);
            controle += vetor[r % n];
        }

        tempo_total = walltime() - inicio;
        if (tempo_total < TEMPO_ALVO) {
            repeticoes *= 2;
        }
    } while (tempo_total < TEMPO_ALVO);

    printf("Teste: inicializacao\n");
    printf("Elementos: %d\n", n);
    printf("Repeticoes: %d\n", repeticoes);
    printf("Tempo total: %.6f s\n", tempo_total);
    printf("Tempo por laco: %.6f s\n", tempo_total / repeticoes);
    printf("Validacao: primeiro=%.3f ultimo=%.3f\n",
           vetor[0], vetor[n - 1]);
}

void medir_soma(const char *teste, double *vetor, int n)
{
    int repeticoes = 1;
    double tempo_total;
    double resultado = 0.0;
    double esperado = (double)n * (n - 1) / 4.0 + n;
    int r;

    inicializar(vetor, n, 0.0);

    do {
        double inicio = walltime();

        for (r = 0; r < repeticoes; r++) {
            resultado = executar_soma(teste, vetor, n);
            controle += resultado;
        }

        tempo_total = walltime() - inicio;
        if (tempo_total < TEMPO_ALVO) {
            repeticoes *= 2;
        }
    } while (tempo_total < TEMPO_ALVO);

    printf("Teste: %s\n", teste);
    printf("Elementos: %d\n", n);
    printf("Repeticoes: %d\n", repeticoes);
    printf("Tempo total: %.6f s\n", tempo_total);
    printf("Tempo por laco: %.6f s\n", tempo_total / repeticoes);
    printf("Resultado: %.1f\n", resultado);
    printf("Erro: %.1f\n", resultado - esperado);
}

int main(int argc, char *argv[])
{
    double *vetor;
    const char *teste;

    if (argc != 2) {
        printf("Uso: %s inicializacao|dependente|multipla2|multipla4|multipla8\n",
               argv[0]);
        return 1;
    }

    teste = argv[1];
    if (strcmp(teste, "inicializacao") != 0 &&
        strcmp(teste, "dependente") != 0 &&
        strcmp(teste, "multipla2") != 0 &&
        strcmp(teste, "multipla4") != 0 &&
        strcmp(teste, "multipla8") != 0) {
        printf("Teste invalido.\n");
        return 1;
    }

    vetor = malloc((size_t)TAMANHO * sizeof(double));
    if (vetor == NULL) {
        printf("Nao foi possivel alocar o vetor.\n");
        return 1;
    }

    if (strcmp(teste, "inicializacao") == 0) {
        medir_inicializacao(vetor, TAMANHO);
    } else {
        medir_soma(teste, vetor, TAMANHO);
    }

    free(vetor);
    return 0;
}
