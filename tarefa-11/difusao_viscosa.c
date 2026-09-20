#define _POSIX_C_SOURCE 199309L

#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    int n;
    double h;
    double viscosidade;
    double dt;
} Malha;

typedef void (*Passo)(const Malha *, const double *, const double *,
                      double *, double *);

typedef struct {
    const char *schedule;
    const char *collapse;
    Passo funcao;
    double segundos;
    int confere;
} Versao;

static double agora(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
}

static double *aloca(int n)
{
    double *p = malloc((size_t)n * (size_t)n * sizeof(double));
    if (p == NULL) {
        fprintf(stderr, "memoria insuficiente\n");
        exit(EXIT_FAILURE);
    }
    return p;
}

static inline void atualiza_celula(const Malha *m,
                                   const double *u, const double *v,
                                   double *un, double *vn, int i, int j)
{
    const int n = m->n;
    const int i_ant = (i + n - 1) % n;
    const int i_prox = (i + 1) % n;
    const int j_ant = (j + n - 1) % n;
    const int j_prox = (j + 1) % n;
    const int centro = i * n + j;
    const double fator = m->viscosidade * m->dt / (m->h * m->h);

    const double laplaciano_u = u[i_prox * n + j] + u[i_ant * n + j]
                              + u[i * n + j_prox] + u[i * n + j_ant]
                              - 4.0 * u[centro];
    const double laplaciano_v = v[i_prox * n + j] + v[i_ant * n + j]
                              + v[i * n + j_prox] + v[i * n + j_ant]
                              - 4.0 * v[centro];

    un[centro] = u[centro] + fator * laplaciano_u;
    vn[centro] = v[centro] + fator * laplaciano_v;
}

static void passo_sequencial(const Malha *m, const double *u, const double *v,
                             double *un, double *vn)
{
    for (int i = 0; i < m->n; i++)
        for (int j = 0; j < m->n; j++)
            atualiza_celula(m, u, v, un, vn, i, j);
}

static void passo_static(const Malha *m, const double *u, const double *v,
                         double *un, double *vn)
{
#pragma omp parallel for schedule(static)
    for (int i = 0; i < m->n; i++)
        for (int j = 0; j < m->n; j++)
            atualiza_celula(m, u, v, un, vn, i, j);
}

static void passo_static_collapse(const Malha *m, const double *u, const double *v,
                                  double *un, double *vn)
{
#pragma omp parallel for collapse(2) schedule(static)
    for (int i = 0; i < m->n; i++)
        for (int j = 0; j < m->n; j++)
            atualiza_celula(m, u, v, un, vn, i, j);
}

static void passo_dynamic(const Malha *m, const double *u, const double *v,
                          double *un, double *vn)
{
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < m->n; i++)
        for (int j = 0; j < m->n; j++)
            atualiza_celula(m, u, v, un, vn, i, j);
}

static void passo_dynamic_collapse(const Malha *m, const double *u, const double *v,
                                   double *un, double *vn)
{
#pragma omp parallel for collapse(2) schedule(dynamic)
    for (int i = 0; i < m->n; i++)
        for (int j = 0; j < m->n; j++)
            atualiza_celula(m, u, v, un, vn, i, j);
}

static void passo_guided(const Malha *m, const double *u, const double *v,
                         double *un, double *vn)
{
#pragma omp parallel for schedule(guided)
    for (int i = 0; i < m->n; i++)
        for (int j = 0; j < m->n; j++)
            atualiza_celula(m, u, v, un, vn, i, j);
}

static void passo_guided_collapse(const Malha *m, const double *u, const double *v,
                                  double *un, double *vn)
{
#pragma omp parallel for collapse(2) schedule(guided)
    for (int i = 0; i < m->n; i++)
        for (int j = 0; j < m->n; j++)
            atualiza_celula(m, u, v, un, vn, i, j);
}

static void simular(const Malha *m, double **u, double **v, double **un, double **vn,
                    int passos, Passo passo)
{
    for (int t = 0; t < passos; t++) {
        passo(m, *u, *v, *un, *vn);
        double *troca;
        troca = *u; *u = *un; *un = troca;
        troca = *v; *v = *vn; *vn = troca;
    }
}

static void preencher(double *campo, int n, double valor)
{
    for (int k = 0; k < n * n; k++)
        campo[k] = valor;
}

static double maior_desvio(const double *campo, int n, double referencia)
{
    double maior = 0.0;
    for (int k = 0; k < n * n; k++) {
        double d = fabs(campo[k] - referencia);
        if (d > maior)
            maior = d;
    }
    return maior;
}

static double soma(const double *campo, int n)
{
    double s = 0.0;
    for (int k = 0; k < n * n; k++)
        s += campo[k];
    return s;
}

static double maximo(const double *campo, int n)
{
    double maior = campo[0];
    for (int k = 1; k < n * n; k++)
        if (campo[k] > maior)
            maior = campo[k];
    return maior;
}

static int comparar_double(const void *a, const void *b)
{
    double x = *(const double *)a;
    double y = *(const double *)b;
    return (x > y) - (x < y);
}

static double mediana(double *v, int n)
{
    qsort(v, (size_t)n, sizeof(double), comparar_double);
    return v[n / 2];
}

static double passo_de_tempo(const Malha *m, double r)
{
    return r * m->h * m->h / m->viscosidade;
}

static void teste_campo_parado(const Malha *m, int passos)
{
    double *u = aloca(m->n), *v = aloca(m->n);
    double *un = aloca(m->n), *vn = aloca(m->n);

    preencher(u, m->n, 0.0);
    preencher(v, m->n, 0.0);
    simular(m, &u, &v, &un, &vn, passos, passo_sequencial);

    printf("1) fluido parado       u=0, v=0        maior |u| = %.3e   maior |v| = %.3e\n",
           maior_desvio(u, m->n, 0.0), maior_desvio(v, m->n, 0.0));

    free(u); free(v); free(un); free(vn);
}

static void teste_campo_constante(const Malha *m, int passos)
{
    const double u0 = 2.0, v0 = -1.0;
    double *u = aloca(m->n), *v = aloca(m->n);
    double *un = aloca(m->n), *vn = aloca(m->n);

    preencher(u, m->n, u0);
    preencher(v, m->n, v0);
    simular(m, &u, &v, &un, &vn, passos, passo_sequencial);

    printf("2) campo constante     u=%.1f, v=%.1f    desvio |u| = %.3e   desvio |v| = %.3e\n",
           u0, v0, maior_desvio(u, m->n, u0), maior_desvio(v, m->n, v0));

    free(u); free(v); free(un); free(vn);
}

static void teste_perturbacao(const Malha *m, int passos)
{
    double *u = aloca(m->n), *v = aloca(m->n);
    double *un = aloca(m->n), *vn = aloca(m->n);

    preencher(u, m->n, 1.0);
    preencher(v, m->n, 0.0);
    u[(m->n / 2) * m->n + m->n / 2] += 1.0;

    double pico_inicial = maximo(u, m->n);
    double soma_inicial = soma(u, m->n);

    simular(m, &u, &v, &un, &vn, passos, passo_sequencial);

    double pico_final = maximo(u, m->n);
    double soma_final = soma(u, m->n);

    printf("3) perturbacao central pico %.6f -> %.6f   soma %.4f -> %.4f (erro %.3e)\n",
           pico_inicial, pico_final, soma_inicial, soma_final,
           fabs(soma_final - soma_inicial));

    free(u); free(v); free(un); free(vn);
}

static void teste_acima_do_limite(const Malha *base, int passos)
{
    const double r_instavel = 0.30;
    Malha m = *base;
    m.dt = passo_de_tempo(&m, r_instavel);

    double *u = aloca(m.n), *v = aloca(m.n);
    double *un = aloca(m.n), *vn = aloca(m.n);

    preencher(u, m.n, 1.0);
    preencher(v, m.n, 0.0);
    u[(m.n / 2) * m.n + m.n / 2] += 1.0;

    simular(&m, &u, &v, &un, &vn, passos, passo_sequencial);

    printf("4) mesmo teste com r = %.2f (acima de 1/4)     pico final = %.3e\n",
           r_instavel, maximo(u, m.n));

    free(u); free(v); free(un); free(vn);
}

static double medir(const Malha *m, int passos, int execucoes, Passo passo,
                    double *saida)
{
    double *tempos = malloc((size_t)execucoes * sizeof(double));
    double *u = aloca(m->n), *v = aloca(m->n);
    double *un = aloca(m->n), *vn = aloca(m->n);

    for (int e = 0; e < execucoes; e++) {
        preencher(u, m->n, 1.0);
        preencher(v, m->n, 0.0);
        u[(m->n / 2) * m->n + m->n / 2] += 1.0;

        double t0 = agora();
        simular(m, &u, &v, &un, &vn, passos, passo);
        tempos[e] = agora() - t0;
    }

    memcpy(saida, u, (size_t)m->n * (size_t)m->n * sizeof(double));

    double t = mediana(tempos, execucoes);
    free(tempos); free(u); free(v); free(un); free(vn);
    return t;
}

int main(int argc, char *argv[])
{
    int n         = (argc > 1) ? atoi(argv[1]) : 1024;
    int passos    = (argc > 2) ? atoi(argv[2]) : 200;
    int threads   = (argc > 3) ? atoi(argv[3]) : omp_get_max_threads();
    int execucoes = (argc > 4) ? atoi(argv[4]) : 5;

    const double r = 0.20;
    Malha m;
    m.n = n;
    m.h = 1.0 / (double)n;
    m.viscosidade = 0.01;
    m.dt = passo_de_tempo(&m, r);

    omp_set_num_threads(threads);

    printf("Malha %d x %d   h = %.3e   viscosidade = %.2f   dt = %.3e   r = %.2f\n",
           n, n, m.h, m.viscosidade, m.dt, r);
    printf("Passos = %d   threads = %d   execucoes = %d\n\n",
           passos, threads, execucoes);

    printf("Validacao (sequencial)\n");
    teste_campo_parado(&m, passos);
    teste_campo_constante(&m, passos);
    teste_perturbacao(&m, passos);
    teste_acima_do_limite(&m, passos);

    printf("\nDesempenho (mediana de %d execucoes)\n", execucoes);

    double *referencia = aloca(n);
    double t_seq = medir(&m, passos, execucoes, passo_sequencial, referencia);

    Versao versoes[6] = {
        { "static",  "nao",         passo_static,           0.0, 0 },
        { "static",  "collapse(2)", passo_static_collapse,  0.0, 0 },
        { "dynamic", "nao",         passo_dynamic,          0.0, 0 },
        { "dynamic", "collapse(2)", passo_dynamic_collapse, 0.0, 0 },
        { "guided",  "nao",         passo_guided,           0.0, 0 },
        { "guided",  "collapse(2)", passo_guided_collapse,  0.0, 0 },
    };

    double *obtido = aloca(n);

    for (int k = 0; k < 6; k++) {
        versoes[k].segundos = medir(&m, passos, execucoes, versoes[k].funcao, obtido);
        versoes[k].confere = (memcmp(obtido, referencia,
                                     (size_t)n * (size_t)n * sizeof(double)) == 0);
    }

    printf("%-10s %-12s %11s %9s %9s\n",
           "schedule", "collapse", "tempo (s)", "speedup", "confere");
    printf("%-10s %-12s %11.4f %9.2f %9s\n", "-", "-", t_seq, 1.0, "-");
    for (int k = 0; k < 6; k++)
        printf("%-10s %-12s %11.4f %9.2f %9s\n",
               versoes[k].schedule, versoes[k].collapse, versoes[k].segundos,
               t_seq / versoes[k].segundos, versoes[k].confere ? "sim" : "NAO");

    free(referencia);
    free(obtido);
    return 0;
}
