#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int processo, processos;
    MPI_Comm_rank(MPI_COMM_WORLD, &processo);
    MPI_Comm_size(MPI_COMM_WORLD, &processos);

    if (argc != 3 || atoi(argv[1]) % processos != 0) {
        if (processo == 0)
            fprintf(stderr, "Uso: %s M N (M multiplo do numero de processos)\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    int m = atoi(argv[1]);
    int n = atoi(argv[2]);
    int linhas = m / processos;   /* linhas de cada processo */

    double *a = NULL, *y = NULL;
    double *x = malloc(n * sizeof(double));
    double *a_local = malloc(linhas * n * sizeof(double));
    double *y_local = malloc(linhas * sizeof(double));

    if (processo == 0) {
        a = malloc(m * n * sizeof(double));
        y = malloc(m * sizeof(double));
        for (int i = 0; i < m; i++)
            for (int j = 0; j < n; j++)
                a[i * n + j] = (i + j) % 10 + 1;
        for (int j = 0; j < n; j++)
            x[j] = 1.0;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();

    MPI_Scatter(a, linhas * n, MPI_DOUBLE, a_local, linhas * n, MPI_DOUBLE,
                0, MPI_COMM_WORLD);
    MPI_Bcast(x, n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    double t1 = MPI_Wtime();

    for (int i = 0; i < linhas; i++) {
        double soma = 0.0;
        for (int j = 0; j < n; j++)
            soma += a_local[i * n + j] * x[j];
        y_local[i] = soma;
    }
    double t2 = MPI_Wtime();

    MPI_Gather(y_local, linhas, MPI_DOUBLE, y, linhas, MPI_DOUBLE,
               0, MPI_COMM_WORLD);
    double t3 = MPI_Wtime();

    /* m, n, processos, total, distribuir, calcular, juntar, y[0], y[m-1] */
    if (processo == 0)
        printf("%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.0f,%.0f\n", m, n, processos,
               t3 - t0, t1 - t0, t2 - t1, t3 - t2, y[0], y[m - 1]);

    free(a);
    free(y);
    free(x);
    free(a_local);
    free(y_local);
    MPI_Finalize();
    return 0;
}
