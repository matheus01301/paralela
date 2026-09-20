#define _POSIX_C_SOURCE 199309L

#include <malloc.h>
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

typedef void (*Inicializador)(const Malha *, double *, double *, double *, double *);
typedef void (*Simulador)(const Malha *, double **, double **, double **, double **, int);

typedef struct {
    const char *nome;
    const char *descricao;
    Inicializador inicializa;
    Simulador simula;
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
        fprintf(stderr, "memoria insuficiente para malha %d\n", n);
        exit(EXIT_FAILURE);
    }
    return p;
}

static inline double fator_de(const Malha *m)
{
    return m->viscosidade * m->dt / (m->h * m->h);
}

static inline void celula_periodica(const Malha *m,
                                    const double *u, const double *v,
                                    double *un, double *vn, int i, int j)
{
    const int n = m->n;
    const int i_ant = (i + n - 1) % n;
    const int i_prox = (i + 1) % n;
    const int j_ant = (j + n - 1) % n;
    const int j_prox = (j + 1) % n;
    const int centro = i * n + j;
    const double fator = fator_de(m);

    const double laplaciano_u = u[i_prox * n + j] + u[i_ant * n + j]
                              + u[i * n + j_prox] + u[i * n + j_ant]
                              - 4.0 * u[centro];
    const double laplaciano_v = v[i_prox * n + j] + v[i_ant * n + j]
                              + v[i * n + j_prox] + v[i * n + j_ant]
                              - 4.0 * v[centro];

    un[centro] = u[centro] + fator * laplaciano_u;
    vn[centro] = v[centro] + fator * laplaciano_v;
}

static void inicializa_sequencial(const Malha *m, double *u, double *v,
                                  double *un, double *vn)
{
    const int total = m->n * m->n;
    for (int k = 0; k < total; k++) {
        u[k] = 1.0;
        v[k] = 0.0;
        un[k] = 0.0;
        vn[k] = 0.0;
    }
    u[(m->n / 2) * m->n + m->n / 2] += 1.0;
}

static void inicializa_first_touch(const Malha *m, double *u, double *v,
                                   double *un, double *vn)
{
    const int n = m->n;
#pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            const int k = i * n + j;
            u[k] = 1.0;
            v[k] = 0.0;
            un[k] = 0.0;
            vn[k] = 0.0;
        }
    u[(n / 2) * n + n / 2] += 1.0;
}

static void troca(double **a, double **b)
{
    double *t = *a;
    *a = *b;
    *b = t;
}

static void passo_modulo(const Malha *m, const double *u, const double *v,
                         double *un, double *vn)
{
#pragma omp parallel for schedule(static)
    for (int i = 0; i < m->n; i++)
        for (int j = 0; j < m->n; j++)
            celula_periodica(m, u, v, un, vn, i, j);
}

static void bordas(const Malha *m, const double *u, const double *v,
                   double *un, double *vn)
{
    const int n = m->n;
    for (int j = 0; j < n; j++) {
        celula_periodica(m, u, v, un, vn, 0, j);
        celula_periodica(m, u, v, un, vn, n - 1, j);
    }
    for (int i = 1; i < n - 1; i++) {
        celula_periodica(m, u, v, un, vn, i, 0);
        celula_periodica(m, u, v, un, vn, i, n - 1);
    }
}

static inline void linha_interior(const Malha *m, const double *u, const double *v,
                                  double *un, double *vn, int i)
{
    const int n = m->n;
    const double fator = fator_de(m);

    const double *u_ant = u + (i - 1) * n;
    const double *u_cen = u + i * n;
    const double *u_prox = u + (i + 1) * n;
    const double *v_ant = v + (i - 1) * n;
    const double *v_cen = v + i * n;
    const double *v_prox = v + (i + 1) * n;
    double *un_cen = un + i * n;
    double *vn_cen = vn + i * n;

    for (int j = 1; j < n - 1; j++) {
        const double laplaciano_u = u_prox[j] + u_ant[j]
                                  + u_cen[j + 1] + u_cen[j - 1]
                                  - 4.0 * u_cen[j];
        const double laplaciano_v = v_prox[j] + v_ant[j]
                                  + v_cen[j + 1] + v_cen[j - 1]
                                  - 4.0 * v_cen[j];
        un_cen[j] = u_cen[j] + fator * laplaciano_u;
        vn_cen[j] = v_cen[j] + fator * laplaciano_v;
    }
}

static void passo_bordas_seriais(const Malha *m, const double *u, const double *v,
                                 double *un, double *vn)
{
#pragma omp parallel for schedule(static)
    for (int i = 1; i < m->n - 1; i++)
        linha_interior(m, u, v, un, vn, i);

    bordas(m, u, v, un, vn);
}

static inline void linha_com_bordas(const Malha *m, const double *u, const double *v,
                                    double *un, double *vn, int i)
{
    const int n = m->n;

    celula_periodica(m, u, v, un, vn, i, 0);
    celula_periodica(m, u, v, un, vn, i, n - 1);

    if (i > 0 && i < n - 1)
        linha_interior(m, u, v, un, vn, i);
    else
        for (int j = 1; j < n - 1; j++)
            celula_periodica(m, u, v, un, vn, i, j);
}

static void passo_bordas_distribuidas(const Malha *m, const double *u, const double *v,
                                      double *un, double *vn)
{
#pragma omp parallel for schedule(static)
    for (int i = 0; i < m->n; i++)
        linha_com_bordas(m, u, v, un, vn, i);
}

static void simula_fork_join(const Malha *m, double **u, double **v,
                             double **un, double **vn, int passos,
                             void (*passo)(const Malha *, const double *, const double *,
                                           double *, double *))
{
    for (int t = 0; t < passos; t++) {
        passo(m, *u, *v, *un, *vn);
        troca(u, un);
        troca(v, vn);
    }
}

static void simula_v0(const Malha *m, double **u, double **v,
                      double **un, double **vn, int passos)
{
    simula_fork_join(m, u, v, un, vn, passos, passo_modulo);
}

static void simula_v2(const Malha *m, double **u, double **v,
                      double **un, double **vn, int passos)
{
    simula_fork_join(m, u, v, un, vn, passos, passo_bordas_seriais);
}

static void simula_v3(const Malha *m, double **u, double **v,
                      double **un, double **vn, int passos)
{
    simula_fork_join(m, u, v, un, vn, passos, passo_bordas_distribuidas);
}

static void simula_v4(const Malha *m, double **pu, double **pv,
                      double **pun, double **pvn, int passos)
{
    double *u = *pu, *v = *pv, *un = *pun, *vn = *pvn;

#pragma omp parallel firstprivate(u, v, un, vn)
    {
        for (int t = 0; t < passos; t++) {
#pragma omp for schedule(static)
            for (int i = 0; i < m->n; i++)
                linha_com_bordas(m, u, v, un, vn, i);

            troca(&u, &un);
            troca(&v, &vn);
        }
    }

    if (passos % 2 == 1) {
        troca(pu, pun);
        troca(pv, pvn);
    }
}

static const Versao VERSOES[] = {
    { "v0", "base da tarefa 11",              inicializa_sequencial,  simula_v0 },
    { "v1", "first touch paralelo",           inicializa_first_touch, simula_v0 },
    { "v2", "interior sem modulo, bordas em serie", inicializa_first_touch, simula_v2 },
    { "v3", "bordas distribuidas nas threads",      inicializa_first_touch, simula_v3 },
    { "v4", "uma unica regiao paralela",            inicializa_first_touch, simula_v4 },
};
static const int N_VERSOES = (int)(sizeof(VERSOES) / sizeof(VERSOES[0]));

static int comparar_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static double mediana(double *v, int n)
{
    qsort(v, (size_t)n, sizeof(double), comparar_double);
    return v[n / 2];
}

static Malha malha_de(int n)
{
    const double r = 0.20;
    Malha m;
    m.n = n;
    m.h = 1.0 / (double)n;
    m.viscosidade = 0.01;
    m.dt = r * m.h * m.h / m.viscosidade;
    return m;
}

static double executar(const Versao *versao, const Malha *m, int passos,
                       int execucoes, double *saida)
{
    double *tempos = malloc((size_t)execucoes * sizeof(double));
    double *u = aloca(m->n), *v = aloca(m->n);
    double *un = aloca(m->n), *vn = aloca(m->n);

    for (int e = 0; e < execucoes; e++) {
        double *a = u, *b = v, *c = un, *d = vn;
        versao->inicializa(m, a, b, c, d);

        double t0 = agora();
        versao->simula(m, &a, &b, &c, &d, passos);
        tempos[e] = agora() - t0;

        if (e == execucoes - 1 && saida != NULL)
            memcpy(saida, a, (size_t)m->n * (size_t)m->n * sizeof(double));
    }

    double t = mediana(tempos, execucoes);
    free(tempos); free(u); free(v); free(un); free(vn);
    return t;
}

static void verificar(int n, int passos)
{
    Malha m = malha_de(n);
    const size_t bytes = (size_t)n * (size_t)n * sizeof(double);
    double *referencia = aloca(n);
    double *obtido = aloca(n);

    omp_set_num_threads(1);
    executar(&VERSOES[0], &m, passos, 1, referencia);

    printf("# verificacao: malha %d, %d passos, campo final comparado com v0 sequencial\n", n, passos);
    const int contagens[] = { 1, 8, 64 };
    for (int k = 0; k < N_VERSOES; k++)
        for (int c = 0; c < 3; c++) {
            omp_set_num_threads(contagens[c]);
            executar(&VERSOES[k], &m, passos, 1, obtido);
            printf("# %s com %3d thread(s): %s\n", VERSOES[k].nome, contagens[c],
                   memcmp(obtido, referencia, bytes) == 0 ? "identico" : "DIFERENTE");
        }

    free(referencia);
    free(obtido);
}

static void escalabilidade_forte(int n, int passos, int execucoes, int max_threads)
{
    Malha m = malha_de(n);

    for (int k = 0; k < N_VERSOES; k++) {
        double t1 = 0.0;
        for (int p = 1; p <= max_threads; p *= 2) {
            omp_set_num_threads(p);
            double t = executar(&VERSOES[k], &m, passos, execucoes, NULL);
            if (p == 1)
                t1 = t;
            printf("forte,%s,%d,%d,%.6f,%.4f,%.4f\n",
                   VERSOES[k].nome, p, n, t, t1 / t, (t1 / t) / (double)p);
            fflush(stdout);
        }
    }
}

static void escalabilidade_fraca(int n_base, int passos, int execucoes, int max_threads)
{
    for (int k = 0; k < N_VERSOES; k++) {
        double t1 = 0.0;
        for (int p = 1; p <= max_threads; p *= 4) {
            int fator = 1;
            for (int q = p; q > 1; q /= 4)
                fator *= 2;
            int n = n_base * fator;

            Malha m = malha_de(n);
            omp_set_num_threads(p);
            double t = executar(&VERSOES[k], &m, passos, execucoes, NULL);
            if (p == 1)
                t1 = t;
            printf("fraca,%s,%d,%d,%.6f,%.4f,%.4f\n",
                   VERSOES[k].nome, p, n, t, (double)p * t1 / t, t1 / t);
            fflush(stdout);
        }
    }
}

int main(int argc, char *argv[])
{
    const char *modo = (argc > 1) ? argv[1] : "forte";
    int n            = (argc > 2) ? atoi(argv[2]) : 2048;
    int passos       = (argc > 3) ? atoi(argv[3]) : 100;
    int execucoes    = (argc > 4) ? atoi(argv[4]) : 3;
    int max_threads  = (argc > 5) ? atoi(argv[5]) : 128;

    mallopt(M_MMAP_THRESHOLD, 128 * 1024);

    if (strcmp(modo, "verificar") == 0) {
        verificar(n, passos);
        return 0;
    }

    printf("# versoes:\n");
    for (int k = 0; k < N_VERSOES; k++)
        printf("#   %s = %s\n", VERSOES[k].nome, VERSOES[k].descricao);
    printf("# passos=%d execucoes=%d (mediana)\n", passos, execucoes);
    printf("modo,versao,threads,n,tempo,speedup,eficiencia\n");

    if (strcmp(modo, "fraca") == 0)
        escalabilidade_fraca(n, passos, execucoes, max_threads);
    else
        escalabilidade_forte(n, passos, execucoes, max_threads);

    return 0;
}
