#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define REPETICOES 1000
#define TAM_MAX (4 * 1024 * 1024)

/* Uma troca: o processo 0 envia e espera a resposta; o processo 1 recebe e
   devolve a mesma mensagem. */
static void ida_e_volta(char *msg, int tam, int processo)
{
    if (processo == 0) {
        MPI_Send(msg, tam, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        MPI_Recv(msg, tam, MPI_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
        MPI_Recv(msg, tam, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(msg, tam, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int processo, processos;
    MPI_Comm_rank(MPI_COMM_WORLD, &processo);
    MPI_Comm_size(MPI_COMM_WORLD, &processos);

    if (processos != 2) {
        if (processo == 0)
            fprintf(stderr, "Execute com exatamente 2 processos\n");
        MPI_Finalize();
        return 1;
    }

    char *msg = calloc(TAM_MAX, 1);

    if (processo == 0)
        printf("bytes,tempo_total_s,tempo_ida_us,banda_MBps\n");

    for (int tam = 8; tam <= TAM_MAX; tam *= 2) {
        /* A primeira troca fica fora da medição: ela inclui o custo de
           preparar a conexão entre os processos. */
        ida_e_volta(msg, tam, processo);

        double inicio = MPI_Wtime();
        for (int r = 0; r < REPETICOES; r++)
            ida_e_volta(msg, tam, processo);
        double total = MPI_Wtime() - inicio;

        if (processo == 0) {
            double ida = total / (2.0 * REPETICOES);
            printf("%d,%.6f,%.3f,%.1f\n", tam, total, ida * 1e6, tam / ida / 1e6);
        }
    }

    free(msg);
    MPI_Finalize();
    return 0;
}
