#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int n;                     /* a malha tem n x n celulas */
static double fator;              /* viscosidade * dt / h^2 */
static double *u, *v, *un, *vn;   /* passo atual (u, v) e proximo (un, vn) */

/* Calcula a celula (i, j) do proximo passo. Bordas periodicas: o % faz a
   malha dar a volta. */
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

/* Um passo da simulacao. As sete versoes tem o mesmo laco; so a diretiva muda. */
static void passo(int versao)
{
    switch (versao) {
    case 0:
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                atualiza_celula(i, j);
        break;
    case 1:
#pragma omp parallel for schedule(static)
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                atualiza_celula(i, j);
        break;
    case 2:
#pragma omp parallel for schedule(static) collapse(2)
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                atualiza_celula(i, j);
        break;
    case 3:
#pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                atualiza_celula(i, j);
        break;
    case 4:
#pragma omp parallel for schedule(dynamic) collapse(2)
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                atualiza_celula(i, j);
        break;
    case 5:
#pragma omp parallel for schedule(guided)
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                atualiza_celula(i, j);
        break;
    case 6:
#pragma omp parallel for schedule(guided) collapse(2)
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                atualiza_celula(i, j);
        break;
    }
}

/* Avanca a simulacao. Depois de cada passo, o proximo vira o atual: so os
   ponteiros sao trocados, sem copiar dados. */
static void simular(int versao, int passos)
{
    for (int t = 0; t < passos; t++) {
        passo(versao);
        double *tmp = u; u = un; un = tmp;
        tmp = v; v = vn; vn = tmp;
    }
}

static void inicia(double u0, double v0)
{
    for (int k = 0; k < n * n; k++) {
        u[k] = u0;
        v[k] = v0;
    }
}

static double maior_desvio(const double *campo, double referencia)
{
    double maior = 0.0;
    for (int k = 0; k < n * n; k++)
        if (fabs(campo[k] - referencia) > maior)
            maior = fabs(campo[k] - referencia);
    return maior;
}

static double soma(const double *campo)
{
    double s = 0.0;
    for (int k = 0; k < n * n; k++)
        s += campo[k];
    return s;
}

static double maximo(const double *campo)
{
    double maior = campo[0];
    for (int k = 1; k < n * n; k++)
        if (campo[k] > maior)
            maior = campo[k];
    return maior;
}

/* Menor tempo entre algumas execucoes da simulacao com a perturbacao central. */
static double medir(int versao, int passos, int execucoes)
{
    double melhor = 1e30;
    for (int e = 0; e < execucoes; e++) {
        inicia(1.0, 0.0);
        u[(n / 2) * n + n / 2] += 1.0;

        double inicio = omp_get_wtime();
        simular(versao, passos);
        double tempo = omp_get_wtime() - inicio;

        if (tempo < melhor)
            melhor = tempo;
    }
    return melhor;
}

int main(int argc, char *argv[])
{
    n = (argc > 1) ? atoi(argv[1]) : 1024;
    int passos = (argc > 2) ? atoi(argv[2]) : 200;
    int threads = (argc > 3) ? atoi(argv[3]) : omp_get_max_threads();
    int execucoes = 5;

    double h = 1.0 / n, viscosidade = 0.01, r = 0.20;
    double dt = r * h * h / viscosidade;   /* Euler explicito: estavel se r <= 1/4 */
    fator = viscosidade * dt / (h * h);

    omp_set_num_threads(threads);
    u = malloc(n * n * sizeof(double));
    v = malloc(n * n * sizeof(double));
    un = malloc(n * n * sizeof(double));
    vn = malloc(n * n * sizeof(double));
    double *referencia = malloc(n * n * sizeof(double));

    printf("Malha %d x %d, %d passos, dt = %.3e, r = %.2f, %d threads\n\n",
           n, n, passos, dt, r, threads);

    printf("Validacao (sequencial)\n");
    inicia(0.0, 0.0);
    simular(0, passos);
    printf("1) fluido parado (u=0, v=0): maior |u| = %.1e, maior |v| = %.1e\n",
           maior_desvio(u, 0.0), maior_desvio(v, 0.0));

    inicia(2.0, -1.0);
    simular(0, passos);
    printf("2) campo constante (u=2, v=-1): desvio u = %.1e, desvio v = %.1e\n",
           maior_desvio(u, 2.0), maior_desvio(v, -1.0));

    inicia(1.0, 0.0);
    u[(n / 2) * n + n / 2] += 1.0;
    double pico0 = maximo(u), soma0 = soma(u);
    simular(0, passos);
    printf("3) perturbacao central: pico %.6f -> %.6f, soma %.4f -> %.4f\n\n",
           pico0, maximo(u), soma0, soma(u));

    const char *nomes[7] = { "sequencial", "static", "static + collapse(2)",
                             "dynamic", "dynamic + collapse(2)",
                             "guided", "guided + collapse(2)" };

    printf("Desempenho (menor tempo de %d execucoes)\n", execucoes);
    printf("versao,tempo_s,speedup,confere\n");
    double t_seq = medir(0, passos, execucoes);
    memcpy(referencia, u, n * n * sizeof(double));
    printf("%s,%.4f,1.00,-\n", nomes[0], t_seq);

    for (int versao = 1; versao <= 6; versao++) {
        double t = medir(versao, passos, execucoes);
        int confere = memcmp(u, referencia, n * n * sizeof(double)) == 0;
        printf("%s,%.4f,%.2f,%s\n", nomes[versao], t, t_seq / t, confere ? "sim" : "NAO");
    }

    free(u); free(v); free(un); free(vn); free(referencia);
    return 0;
}
