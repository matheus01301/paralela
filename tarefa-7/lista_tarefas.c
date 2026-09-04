#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct No {
    char nome[64];
    int processamentos;
    struct No *proximo;
} No;

static No *criar_lista(const char *nomes[], int quantidade) {
    No *inicio = NULL;
    No *fim = NULL;

    for (int i = 0; i < quantidade; i++) {
        No *novo = malloc(sizeof(*novo));
        if (novo == NULL) {
            while (inicio != NULL) {
                No *seguinte = inicio->proximo;
                free(inicio);
                inicio = seguinte;
            }
            return NULL;
        }

        snprintf(novo->nome, sizeof(novo->nome), "%s", nomes[i]);
        novo->processamentos = 0;
        novo->proximo = NULL;

        if (inicio == NULL) {
            inicio = novo;
        } else {
            fim->proximo = novo;
        }
        fim = novo;
    }

    return inicio;
}

static void liberar_lista(No *inicio) {
    while (inicio != NULL) {
        No *seguinte = inicio->proximo;
        free(inicio);
        inicio = seguinte;
    }
}

static void zerar_contadores(No *inicio) {
    for (No *atual = inicio; atual != NULL; atual = atual->proximo) {
        atual->processamentos = 0;
    }
}

static void processar_arquivo(No *no, const char *versao) {
    int ocorrencia;
    int thread = omp_get_thread_num();

    /* O contador serve apenas para verificar repeticoes no experimento. */
    #pragma omp atomic capture
    ocorrencia = ++no->processamentos;

    /* Evita que duas linhas de printf se misturem no terminal. */
    #pragma omp critical(saida)
    {
        printf("[%s] %-20s | thread executora %d | ocorrencia %d\n",
               versao, no->nome, thread, ocorrencia);
    }
}

/*
 * Versao propositalmente incorreta: toda thread executa o mesmo laco.
 * Com T threads, cada no gera T tarefas e e processado T vezes.
 */
static void executar_sem_single(No *inicio) {
    #pragma omp parallel default(none) shared(inicio)
    {
        for (No *atual = inicio; atual != NULL; atual = atual->proximo) {
            #pragma omp task firstprivate(atual)
            processar_arquivo(atual, "sem single");
        }
    }
}

/*
 * Versao correta: uma unica thread percorre a lista e produz as tarefas.
 * A equipe inteira continua disponivel para executar essas tarefas.
 */
static void executar_com_single(No *inicio) {
    #pragma omp parallel default(none) shared(inicio)
    {
        #pragma omp single
        {
            for (No *atual = inicio; atual != NULL; atual = atual->proximo) {
                #pragma omp task firstprivate(atual)
                processar_arquivo(atual, "com single");
            }

            #pragma omp taskwait
        }
    }
}

static int mostrar_resumo(const No *inicio) {
    int diferentes_de_um = 0;

    puts("Resumo da rodada:");
    for (const No *atual = inicio; atual != NULL; atual = atual->proximo) {
        printf("  %-20s -> %d processamento(s)\n",
               atual->nome, atual->processamentos);
        if (atual->processamentos != 1) {
            diferentes_de_um++;
        }
    }
    return diferentes_de_um;
}

int main(int argc, char *argv[]) {
    const char *nomes[] = {
        "relatorio.pdf",
        "dados.csv",
        "imagem.png",
        "notas.txt",
        "programa.c",
        "apresentacao.pptx"
    };
    const int quantidade = (int)(sizeof(nomes) / sizeof(nomes[0]));
    int threads = (argc > 1) ? atoi(argv[1]) : 4;

    if (threads < 1) {
        fprintf(stderr, "Uso: %s [numero_de_threads >= 1]\n", argv[0]);
        return EXIT_FAILURE;
    }

    No *lista = criar_lista(nomes, quantidade);
    if (lista == NULL) {
        fputs("Nao foi possivel criar a lista.\n", stderr);
        return EXIT_FAILURE;
    }

    omp_set_dynamic(0);
    omp_set_num_threads(threads);

    printf("Lista com %d nos; equipe solicitada com %d threads.\n\n",
           quantidade, threads);

    puts("=== 1. SEM single: erro proposital ===");
    executar_sem_single(lista);
    int erros_sem_single = mostrar_resumo(lista);

    zerar_contadores(lista);
    puts("\n=== 2. COM single: versao correta ===");
    executar_com_single(lista);
    int erros_com_single = mostrar_resumo(lista);

    printf("\nDiagnostico: sem single, %d de %d nos nao foram processados uma vez.\n",
           erros_sem_single, quantidade);
    printf("Diagnostico: com single, %d de %d nos nao foram processados uma vez.\n",
           erros_com_single, quantidade);

    liberar_lista(lista);
    return (erros_com_single == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
