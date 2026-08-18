/* ============================================================================
 * pi_serie.c - Aproximacao de PI por series matematicas
 *
 * Objetivo: variar o numero de iteracoes, medir o tempo de execucao e observar
 * como o erro em relacao ao valor real de PI diminui conforme gastamos mais
 * processamento.
 *
 * Duas series sao comparadas:
 *   Leibniz-Gregory : pi = 4 * (1 - 1/3 + 1/5 - 1/7 + ...)      erro ~ 1/N
 *   Nilakantha      : pi = 3 + 4/(2*3*4) - 4/(4*5*6) + ...      erro ~ 1/(4N^3)
 *
 * Compilar:  gcc -O2 -Wall -Wextra -std=c99 pi_serie.c -o pi_serie -lm
 * Executar:  ./pi_serie          (vai ate 10^9 iteracoes)
 *            ./pi_serie 7        (vai ate 10^7 iteracoes)
 * ==========================================================================*/

#define _POSIX_C_SOURCE 199309L   /* libera clock_gettime() no Linux/macOS */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
#endif

/* Valor de referencia de PI com mais digitos do que um double consegue guardar
 * (um double tem ~15-17 digitos decimais significativos). Nao usamos M_PI
 * porque ele nao faz parte do C padrao - so existe como extensao. */
static const double PI_REF = 3.14159265358979323846;

/* Quantos valores de N vamos testar, no maximo (10^1 ate 10^9). */
#define MAX_EXPOENTE 9

/* ---------------------------------------------------------------------------
 * MEDICAO DE TEMPO
 *
 * Existem DOIS tempos diferentes, e a diferenca entre eles e' o coracao da
 * disciplina de programacao paralela:
 *
 *   tempo de parede (wall time) : o relogio da parede, o que o usuario espera
 *   tempo de CPU               : soma do tempo que os nucleos ficaram ocupados
 *
 * Em codigo sequencial os dois sao praticamente iguais. Em codigo paralelo com
 * 8 threads, 1 segundo de parede pode custar 8 segundos de CPU. O speedup e'
 * sempre calculado com tempo de PAREDE.
 * -------------------------------------------------------------------------*/

/* Tempo de parede, em segundos, com resolucao de nanosegundos.
 * O #if e' so portabilidade: Windows e POSIX tem APIs diferentes. */
static double tempo_parede(void)
{
#if defined(_WIN32)
    LARGE_INTEGER freq, agora;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&agora);
    return (double)agora.QuadPart / (double)freq.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
#endif
}

/* Tempo de CPU consumido pelo processo, em segundos.
 * clock() devolve "ticks"; CLOCKS_PER_SEC converte para segundos. */
static double tempo_cpu(void)
{
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

/* ---------------------------------------------------------------------------
 * AS SERIES
 * -------------------------------------------------------------------------*/

/* Serie de Leibniz-Gregory:
 *
 *     pi/4 = 1 - 1/3 + 1/5 - 1/7 + 1/9 - ...
 *
 * O termo k (comecando em 0) e' (-1)^k / (2k+1).
 *
 * Note que NAO usamos pow(-1.0, k) para alternar o sinal: pow() e' uma chamada
 * de funcao cara (dezenas de ciclos) e seria executada bilhoes de vezes.
 * Multiplicar uma variavel por -1 custa 1 ciclo. Trocar pow() por isso deixa o
 * laco varias vezes mais rapido sem mudar o resultado. */
static double serie_leibniz(long long n)
{
    double soma  = 0.0;
    double sinal = 1.0;
    long long k;

    for (k = 0; k < n; k++) {
        soma += sinal / (double)(2 * k + 1);
        sinal = -sinal;
    }
    return 4.0 * soma;
}

/* Serie de Nilakantha:
 *
 *     pi = 3 + 4/(2*3*4) - 4/(4*5*6) + 4/(6*7*8) - ...
 *
 * O termo k (comecando em 1) usa a = 2k e vale (-1)^(k+1) * 4/(a(a+1)(a+2)).
 * O denominador cresce com k^3, entao o erro cai com 1/N^3 - muito mais rapido
 * que Leibniz. Serve para mostrar que trocar de ALGORITMO costuma valer mais
 * que empilhar iteracoes (ou nucleos) em cima de um algoritmo ruim. */
static double serie_nilakantha(long long n)
{
    double soma  = 3.0;
    double sinal = 1.0;
    long long k;

    for (k = 1; k <= n; k++) {
        double a = 2.0 * (double)k;
        soma += sinal * 4.0 / (a * (a + 1.0) * (a + 2.0));
        sinal = -sinal;
    }
    return soma;
}

/* ---------------------------------------------------------------------------
 * APOIO
 * -------------------------------------------------------------------------*/

/* Quantas casas decimais estao corretas, aproximadamente.
 * Se o erro e' 1e-5, entao -log10(1e-5) = 5 casas corretas. */
static double casas_corretas(double erro)
{
    if (erro <= 0.0) return 17.0;          /* erro zero: limite do double */
    return -log10(erro);
}

/* Executa uma serie para um N, mede os tempos e imprime a linha da tabela.
 * Recebe um ponteiro para funcao para nao duplicar todo esse codigo de medicao
 * uma vez para Leibniz e outra para Nilakantha. */
static void executa(const char *nome, double (*serie)(long long), long long n,
                    double *erro_saida, double *tempo_saida)
{
    double t0_parede, t1_parede, t0_cpu, t1_cpu;
    double aprox, erro;

    t0_parede = tempo_parede();
    t0_cpu    = tempo_cpu();

    aprox = serie(n);                       /* <-- o trabalho de verdade */

    t1_cpu    = tempo_cpu();
    t1_parede = tempo_parede();

    erro = fabs(PI_REF - aprox);

    printf("  %-11s %12lld  %.15f  %10.3e  %6.2f  %9.4f  %9.4f\n",
           nome, n, aprox, erro, casas_corretas(erro),
           t1_parede - t0_parede, t1_cpu - t0_cpu);

    *erro_saida  = erro;
    *tempo_saida = t1_parede - t0_parede;
}

/* ---------------------------------------------------------------------------
 * PROGRAMA PRINCIPAL
 * -------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    int expoente_max = MAX_EXPOENTE;
    int e, i;

    /* Arrays para guardar os resultados e analisar a convergencia no final. */
    long long ns[MAX_EXPOENTE + 1];
    double erro_leib[MAX_EXPOENTE + 1], tempo_leib[MAX_EXPOENTE + 1];
    double erro_nila[MAX_EXPOENTE + 1], tempo_nila[MAX_EXPOENTE + 1];
    int qtd = 0;

    /* argv[1] opcional: expoente maximo, para nao esperar muito em teste. */
    if (argc > 1) {
        expoente_max = atoi(argv[1]);
        if (expoente_max < 1 || expoente_max > MAX_EXPOENTE) {
            fprintf(stderr, "Uso: %s [expoente entre 1 e %d]\n",
                    argv[0], MAX_EXPOENTE);
            return 1;
        }
    }

    printf("Valor de referencia de PI : %.15f\n", PI_REF);
    printf("Precisao do tipo double   : ~15-17 digitos decimais\n\n");

    printf("  %-11s %12s  %-17s %10s  %6s  %9s  %9s\n",
           "SERIE", "N", "APROXIMACAO", "ERRO ABS", "CASAS", "T.PAREDE", "T.CPU");
    printf("  ---------------------------------------------------------"
           "-------------------------------------\n");

    for (e = 1; e <= expoente_max; e++) {
        long long n = 1;
        int j;
        for (j = 0; j < e; j++) n *= 10;    /* n = 10^e sem usar pow() */

        ns[qtd] = n;
        executa("Leibniz",    serie_leibniz,    n, &erro_leib[qtd], &tempo_leib[qtd]);
        executa("Nilakantha", serie_nilakantha, n, &erro_nila[qtd], &tempo_nila[qtd]);
        printf("\n");
        qtd++;
    }

    /* ---- Analise da convergencia -----------------------------------------
     * A cada passo multiplicamos N por 10. Se o erro cai por um fator ~10, a
     * convergencia e' de 1a ordem (erro ~ 1/N). Se cai por ~1000, e' de 3a
     * ordem (erro ~ 1/N^3). O tempo, em ambos os casos, cresce ~10x, porque o
     * custo e' O(N) nas duas series. */
    printf("Fator de reducao do erro e de aumento do tempo a cada 10x em N:\n\n");
    printf("  %14s  %12s  %12s  %12s\n",
           "N", "ERRO/10x LEIB", "ERRO/10x NILA", "TEMPO/10x");
    printf("  ------------------------------------------------------------\n");
    for (i = 1; i < qtd; i++) {
        double fator_leib  = erro_leib[i - 1]  / erro_leib[i];
        double fator_nila  = erro_nila[i - 1]  / erro_nila[i];
        double fator_tempo = (tempo_leib[i - 1] > 0.0)
                             ? tempo_leib[i] / tempo_leib[i - 1] : 0.0;
        printf("  %14lld  %12.1f  %12.1f  %12.1f\n",
               ns[i], fator_leib, fator_nila, fator_tempo);
    }

    printf("\nLeitura: com Leibniz, 10x mais trabalho compra ~1 casa decimal.\n");
    printf("Com Nilakantha, o mesmo trabalho compra ~3 casas - ate a serie\n");
    printf("bater no limite de precisao do double (~1e-16), onde mais\n");
    printf("iteracoes so acumulam erro de arredondamento e nao melhoram nada.\n");

    return 0;
}
