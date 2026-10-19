#define _GNU_SOURCE   /* para sched_getcpu */

#include <omp.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

static int n;                     /* a malha tem n x n celulas */
static double fator;              /* viscosidade * dt / h^2 */
static double *u, *v, *un, *vn;   /* passo atual (u, v) e proximo (un, vn) */

/* Mesma conta da Tarefa 11: celula (i, j) do proximo passo, bordas periodicas. */
static inline void atualiza_celula(int i, int j)
{
    int cima = (i + n - 1) % n, baixo = (i + 1) % n;
    int esq = (j + n - 1) % n, dir = (j + 1) % n;
    int c = i * n + j;

    double lap_u = u[cima * n + j] + u[baixo * n + j] + u[i * n + esq] + u[i * n + dir]
                 - 4.0 * u[c];
    double lap_v = v[cima * n + j] + v[baixo * n + j] + v[i * n + esq] + v[i * n + dir]
                 - 4.0 * v[c];

    un[c] = u[c] + fator * lap_u;
    vn[c] = v[c] + fator * lap_v;
}

/* Inicializa em paralelo, com a mesma divisao do calculo: cada thread toca
   primeiro as linhas que vai calcular, e a memoria fica perto dela. */
static void inicia(void)
{
#pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            u[i * n + j] = 1.0;
            v[i * n + j] = 0.0;
            un[i * n + j] = 0.0;
            vn[i * n + j] = 0.0;
        }
    u[(n / 2) * n + n / 2] += 1.0;
}

static void simular(int passos)
{
    for (int t = 0; t < passos; t++) {
#pragma omp parallel for schedule(static)
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                atualiza_celula(i, j);

        double *tmp = u; u = un; un = tmp;
        tmp = v; v = vn; vn = tmp;
    }
}

/* Mostra em qual CPU cada thread ficou. */
static void mostra_cpus(int threads)
{
    int cpu[128];
    omp_set_num_threads(threads);
#pragma omp parallel
    cpu[omp_get_thread_num()] = sched_getcpu();

    printf("# cpus com %d threads:", threads);
    for (int t = 0; t < threads; t++)
        printf(" %d", cpu[t]);
    printf("\n");
}

int main(int argc, char *argv[])
{
    const char *rotulo = (argc > 1) ? argv[1] : "teste";
    int max_threads = (argc > 2) ? atoi(argv[2]) : 128;
    int passos = 200, execucoes = 5;

    n = 2048;
    double h = 1.0 / n, viscosidade = 0.01, r = 0.20;
    double dt = r * h * h / viscosidade;
    fator = viscosidade * dt / (h * h);

    u = malloc(n * n * sizeof(double));
    v = malloc(n * n * sizeof(double));
    un = malloc(n * n * sizeof(double));
    vn = malloc(n * n * sizeof(double));

    mostra_cpus(8);

    /* rotulo, threads, menor tempo de 5 execucoes */
    for (int p = 1; p <= max_threads; p *= 2) {
        omp_set_num_threads(p);
        double melhor = 1e30;
        for (int e = 0; e < execucoes; e++) {
            inicia();
            double inicio = omp_get_wtime();
            simular(passos);
            double tempo = omp_get_wtime() - inicio;
            if (tempo < melhor)
                melhor = tempo;
        }
        printf("%s,%d,%.4f\n", rotulo, p, melhor);
        fflush(stdout);
    }

    free(u); free(v); free(un); free(vn);
    return 0;
}
