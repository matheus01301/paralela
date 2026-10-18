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
    int colunas_locais = (n + processos - 1) / processos;
    int colunas_distribuidas = colunas_locais * processos;

    double *a = NULL;
    double *x = NULL;
    double *y = NULL;
    double *a_local = malloc((size_t)m * colunas_locais * sizeof(double));
    double *x_local = malloc((size_t)colunas_locais * sizeof(double));
    double *y_local = malloc((size_t)m * sizeof(double));

    if (processo == 0) {
        a = calloc((size_t)m * colunas_distribuidas, sizeof(double));
        x = calloc((size_t)colunas_distribuidas, sizeof(double));
        y = malloc((size_t)m * sizeof(double));
    }
    if (!a_local || !x_local || !y_local ||
        (processo == 0 && (!a || !x || !y))) {
        fprintf(stderr, "Processo %d: memoria insuficiente\n", processo);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (processo == 0) {
        for (int j = 0; j < n; j++) {
            x[j] = 1.0;
        }
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                a[(size_t)i * colunas_distribuidas + j] =
                    (double)((i + j) % 10 + 1);
            }
        }
    }

    MPI_Datatype tipo_colunas, tipo_redimensionado;
    MPI_Type_vector(m, colunas_locais, colunas_distribuidas,
                    MPI_DOUBLE, &tipo_colunas);
    MPI_Type_create_resized(tipo_colunas, 0,
                            (MPI_Aint)colunas_locais * sizeof(double),
                            &tipo_redimensionado);
    MPI_Type_commit(&tipo_redimensionado);
    MPI_Type_free(&tipo_colunas);

    MPI_Barrier(MPI_COMM_WORLD);
    double inicio = MPI_Wtime();

    MPI_Scatter(a, 1, tipo_redimensionado,
                a_local, m * colunas_locais, MPI_DOUBLE,
                0, MPI_COMM_WORLD);
    MPI_Scatter(x, colunas_locais, MPI_DOUBLE,
                x_local, colunas_locais, MPI_DOUBLE,
                0, MPI_COMM_WORLD);

    for (int i = 0; i < m; i++) {
        double soma = 0.0;
        for (int j = 0; j < colunas_locais; j++) {
            soma += a_local[(size_t)i * colunas_locais + j] * x_local[j];
        }
        y_local[i] = soma;
    }

    MPI_Reduce(y_local, y, m, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (processo == 0) {
        double tempo = MPI_Wtime() - inicio;
        printf("M=%d N=%d processos=%d tempo=%.6f s y[0]=%.0f y[%d]=%.0f\n",
               m, n, processos, tempo, y[0], m - 1, y[m - 1]);
    }

    MPI_Type_free(&tipo_redimensionado);
    free(a);
    free(x);
    free(y);
    free(a_local);
    free(x_local);
    free(y_local);
    MPI_Finalize();
    return 0;
}
