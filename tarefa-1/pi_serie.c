/*
 * Tarefa 1 - Aproximacao do valor de PI
 *
 * Compilar:
 *   gcc -O2 -Wall -Wextra pi_serie.c -o pi_serie.exe -lm
 */

#include <stdio.h>
#include <time.h>
#include <math.h>

#define PI_REAL 3.141592653589793

/* Calcula PI pela serie de Leibniz:
 * PI = 4 * (1 - 1/3 + 1/5 - 1/7 + ...)
 */
double calcular_pi(int iteracoes)
{
    double soma = 0.0;
    double sinal = 1.0;
    int i;

    for (i = 0; i < iteracoes; i++) {
        soma += sinal / (2 * i + 1);
        sinal = -sinal;
    }

    return 4.0 * soma;
}

int main(void)
{
    int testes[] = {
        1000000,
        10000000,
        100000000,
        1000000000
    };
    int i;

    printf("%-12s %-18s %-12s %s\n",
           "Iteracoes", "Aproximacao", "Erro", "Tempo (s)");

    for (i = 0; i < 4; i++) {
        clock_t inicio = clock();
        double pi = calcular_pi(testes[i]);
        double tempo = (double)(clock() - inicio) / CLOCKS_PER_SEC;
        double erro = fabs(PI_REAL - pi);

        printf("%-12d %.15f  %.3e    %.4f\n",
               testes[i], pi, erro, tempo);
    }

    return 0;
}
