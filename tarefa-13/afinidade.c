#define _GNU_SOURCE

#include <dirent.h>
#include <malloc.h>
#include <math.h>
#include <omp.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_THREADS 512

typedef struct {
    int n;
    double h;
    double viscosidade;
    double dt;
} Malha;

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

/* ---------------------------------------------------- topologia observada */

static int primeiro_id(const char *lista)
{
    int v = -1;
    sscanf(lista, "%d", &v);
    return v;
}

static int dominio_l3(int cpu)
{
    char caminho[128], conteudo[4096];
    snprintf(caminho, sizeof caminho,
             "/sys/devices/system/cpu/cpu%d/cache/index3/shared_cpu_list", cpu);
    FILE *f = fopen(caminho, "r");
    if (f == NULL)
        return -1;
    if (fgets(conteudo, sizeof conteudo, f) == NULL)
        conteudo[0] = '\0';
    fclose(f);
    return primeiro_id(conteudo);
}

static int no_numa(int cpu)
{
    char caminho[128];
    snprintf(caminho, sizeof caminho, "/sys/devices/system/cpu/cpu%d", cpu);
    DIR *d = opendir(caminho);
    if (d == NULL)
        return -1;
    int no = -1;
    for (struct dirent *e = readdir(d); e != NULL; e = readdir(d)) {
        int v;
        if (sscanf(e->d_name, "node%d", &v) == 1) {
            no = v;
            break;
        }
    }
    closedir(d);
    return no;
}

static int distintos(const int *v, int n)
{
    int total = 0;
    for (int i = 0; i < n; i++) {
        int repetido = 0;
        for (int j = 0; j < i; j++)
            if (v[j] == v[i]) {
                repetido = 1;
                break;
            }
        if (!repetido)
            total++;
    }
    return total;
}

static void observar(int threads, int *cpus)
{
    for (int i = 0; i < threads; i++)
        cpus[i] = -1;

    omp_set_num_threads(threads);
#pragma omp parallel
    {
        int id = omp_get_thread_num();
        if (id < threads)
            cpus[id] = sched_getcpu();
    }
}

/* -------------------------------------------------------------- simulacao */

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

static void troca(double **a, double **b)
{
    double *t = *a;
    *a = *b;
    *b = t;
}

static void inicializa(const Malha *m, double *u, double *v, double *un, double *vn)
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

static void simular(const Malha *m, double **pu, double **pv,
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

/* ---------------------------------------------------------------- medicao */

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

static double medir(const Malha *m, int passos, int execucoes)
{
    double *tempos = malloc((size_t)execucoes * sizeof(double));
    double *u = aloca(m->n), *v = aloca(m->n);
    double *un = aloca(m->n), *vn = aloca(m->n);

    for (int e = 0; e < execucoes; e++) {
        double *a = u, *b = v, *c = un, *d = vn;
        inicializa(m, a, b, c, d);

        double t0 = agora();
        simular(m, &a, &b, &c, &d, passos);
        tempos[e] = agora() - t0;
    }

    double t = mediana(tempos, execucoes);
    free(tempos); free(u); free(v); free(un); free(vn);
    return t;
}

static void imprimir_mapa(const char *rotulo, int threads)
{
    int cpus[MAX_THREADS];
    observar(threads, cpus);

    printf("# mapa %s com %d threads\n#  thread:cpu ", rotulo, threads);
    for (int i = 0; i < threads && i < 32; i++)
        printf("%d:%d ", i, cpus[i]);
    if (threads > 32)
        printf("...");
    printf("\n");
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "uso: %s <rotulo> <n> [passos] [execucoes] [lista_threads]\n", argv[0]);
        return 1;
    }

    mallopt(M_MMAP_THRESHOLD, 128 * 1024);

    const char *rotulo = argv[1];
    int n         = atoi(argv[2]);
    int passos    = (argc > 3) ? atoi(argv[3]) : 200;
    int execucoes = (argc > 4) ? atoi(argv[4]) : 5;
    const char *lista = (argc > 5) ? argv[5] : "1,4,8,16,32,64,128";

    Malha m = malha_de(n);

    int contagens[64], total = 0;
    for (const char *p = lista; *p && total < 64; ) {
        contagens[total++] = atoi(p);
        while (*p && *p != ',') p++;
        if (*p == ',') p++;
    }

    const char *base = getenv("TEMPO_BASE");
    double t1 = (base != NULL) ? atof(base) : 0.0;
    int cpus[MAX_THREADS], l3[MAX_THREADS], numa[MAX_THREADS];

    for (int k = 0; k < total; k++) {
        int p = contagens[k];

        observar(p, cpus);
        for (int i = 0; i < p; i++) {
            l3[i] = dominio_l3(cpus[i]);
            numa[i] = no_numa(cpus[i]);
        }

        double t = medir(&m, passos, execucoes);
        if (t1 == 0.0)
            t1 = t;

        printf("%s,%d,%d,%.6f,%.4f,%.4f,%d,%d,%d\n",
               rotulo, p, n, t, t1 / t, (t1 / t) / (double)p,
               distintos(l3, p), distintos(numa, p), distintos(cpus, p));
        fflush(stdout);
    }

    if (getenv("MAPA") != NULL)
        imprimir_mapa(rotulo, 8);

    return 0;
}
