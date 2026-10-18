#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int processo, processos;
    MPI_Comm_rank(MPI_COMM_WORLD, &processo);
    MPI_Comm_size(MPI_COMM_WORLD, &processos);

    if (argc != 3 || atoi(argv[1]) <= 0 || atoi(argv[2]) <= 0) {
        if (processo == 0) {
            fprintf(stderr, "Uso: %s M N (ambos positivos)\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    int m = atoi(argv[1]);
    int n = atoi(argv[2]);
    int linhas_locais = (m + processos - 1) / processos;
    int linhas_distribuidas = linhas_locais * processos;

    double *a = NULL;
    double *y = NULL;
    double *x = malloc((size_t)n * sizeof(double));
    double *a_local = malloc((size_t)linhas_locais * n * sizeof(double));
    double *y_local = malloc((size_t)linhas_locais * sizeof(double));

    if (processo == 0) {
        a = calloc((size_t)linhas_distribuidas * n, sizeof(double));
        y = malloc((size_t)linhas_distribuidas * sizeof(double));
    }
    if (!x || !a_local || !y_local || (processo == 0 && (!a || !y))) {
        fprintf(stderr, "Processo %d: memoria insuficiente\n", processo);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (processo == 0) {
        for (int j = 0; j < n; j++) {
            x[j] = 1.0;
        }
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                a[(size_t)i * n + j] = (double)((i + j) % 10 + 1);
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double inicio = MPI_Wtime();

    MPI_Scatter(a, linhas_locais * n, MPI_DOUBLE,
                a_local, linhas_locais * n, MPI_DOUBLE,
                0, MPI_COMM_WORLD);
    MPI_Bcast(x, n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    for (int i = 0; i < linhas_locais; i++) {
        double soma = 0.0;
        for (int j = 0; j < n; j++) {
            soma += a_local[(size_t)i * n + j] * x[j];
        }
        y_local[i] = soma;
    }

    MPI_Gather(y_local, linhas_locais, MPI_DOUBLE,
               y, linhas_locais, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (processo == 0) {
        double tempo = MPI_Wtime() - inicio;
        printf("M=%d N=%d processos=%d tempo=%.6f s y[0]=%.0f y[%d]=%.0f\n",
               m, n, processos, tempo, y[0], m - 1, y[m - 1]);
    }

    free(a);
    free(y);
    free(x);
    free(a_local);
    free(y_local);
    MPI_Finalize();
    return 0;
}
