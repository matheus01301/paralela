/*
 * Tarefa 9 - tarefas, listas encadeadas, critical nomeado e locks
 *
 * Compilar:
 *   gcc -O2 -Wall -Wextra -std=c11 -fopenmp listas_insercoes.c -o listas_insercoes.exe
 *
 * Executar:
 *   listas_insercoes.exe [insercoes] [threads] [listas] [semente]
 */

#include <errno.h>
#include <limits.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CACHE_LINE 64

typedef struct No {
    long long valor;
    struct No *proximo;
} No;

/* A separacao evita que os cabecalhos de duas listas disputem a mesma linha. */
typedef struct {
    No *inicio;
    size_t tamanho;
    unsigned char separacao[CACHE_LINE];
} Lista;

/* Instancias consecutivas ficam a mais de uma linha de cache de distancia. */
typedef struct {
    omp_lock_t valor;
    unsigned char separacao[CACHE_LINE];
} LockSeparado;

_Static_assert(sizeof(LockSeparado) > CACHE_LINE,
               "locks consecutivos precisam ficar separados");

typedef struct {
    double segundos;
    long long inseridos;
    long long falhas_alocacao;
    int valido;
} Resultado;

typedef enum {
    CRITICAL_COMUM,
    CRITICAL_NOMEADO
} ModoDuasListas;

/* Misturador deterministico: cada tarefa tem seu proprio estado, sem rand(). */
static uint64_t misturar(uint64_t x)
{
    x += UINT64_C(0x9e3779b97f4a7c15);
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    return x ^ (x >> 31);
}

static int escolher_lista(long long tarefa, unsigned int semente, int quantidade)
{
    uint64_t estado = (uint64_t)tarefa ^ ((uint64_t)semente << 32);
    return (int)(misturar(estado) % (uint64_t)quantidade);
}

/* Deve ser chamada somente enquanto a protecao da lista estiver adquirida. */
static void inserir_sem_protecao(Lista *lista, No *novo)
{
    novo->proximo = lista->inicio;
    lista->inicio = novo;
    lista->tamanho++;
}

static void inserir_critical_comum(Lista listas[2], int destino, No *novo)
{
    /* O critical sem nome usa o mesmo lock implicito para as duas listas. */
    #pragma omp critical
    inserir_sem_protecao(&listas[destino], novo);
}

static void inserir_critical_nomeado(Lista listas[2], int destino, No *novo)
{
    /* Os nomes representam locks implicitos diferentes e conhecidos em compilacao. */
    if (destino == 0) {
        #pragma omp critical(lista_zero)
        inserir_sem_protecao(&listas[0], novo);
    } else {
        #pragma omp critical(lista_um)
        inserir_sem_protecao(&listas[1], novo);
    }
}

static void liberar_listas(Lista *listas, int quantidade)
{
    for (int i = 0; i < quantidade; i++) {
        No *atual = listas[i].inicio;
        while (atual != NULL) {
            No *seguinte = atual->proximo;
            free(atual);
            atual = seguinte;
        }
    }
}

static int validar_listas(const Lista *listas, int quantidade,
                          long long esperado, long long *encontrados)
{
    long long total_cabecalhos = 0;
    long long total_percorrido = 0;
    int valido = 1;

    for (int i = 0; i < quantidade; i++) {
        size_t percorridos = 0;
        for (const No *atual = listas[i].inicio;
             atual != NULL; atual = atual->proximo) {
            percorridos++;
        }
        if (percorridos != listas[i].tamanho) {
            valido = 0;
        }
        total_cabecalhos += (long long)listas[i].tamanho;
        total_percorrido += (long long)percorridos;
    }

    *encontrados = total_percorrido;
    return valido && total_cabecalhos == esperado &&
           total_percorrido == esperado;
}

static Resultado executar_duas_listas(long long insercoes,
                                      unsigned int semente,
                                      ModoDuasListas modo)
{
    Lista listas[2] = {0};
    long long falhas = 0;
    double inicio = omp_get_wtime();

    #pragma omp parallel default(none) \
        shared(listas, falhas, insercoes, semente, modo)
    {
        #pragma omp single
        {
            #pragma omp taskgroup
            {
                for (long long i = 0; i < insercoes; i++) {
                    #pragma omp task firstprivate(i)
                    {
                        int destino = escolher_lista(i, semente, 2);
                        No *novo = malloc(sizeof(*novo));

                        if (novo == NULL) {
                            #pragma omp atomic update
                            falhas++;
                        } else {
                            novo->valor = i;
                            if (modo == CRITICAL_NOMEADO) {
                                inserir_critical_nomeado(listas, destino, novo);
                            } else {
                                inserir_critical_comum(listas, destino, novo);
                            }
                        }
                    }
                }
            }
        }
    }

    Resultado r;
    r.segundos = omp_get_wtime() - inicio;
    r.falhas_alocacao = falhas;
    r.valido = validar_listas(listas, 2, insercoes - falhas, &r.inseridos);
    liberar_listas(listas, 2);
    return r;
}

static Resultado executar_varias_listas(long long insercoes,
                                        unsigned int semente,
                                        int quantidade)
{
    Lista *listas = calloc((size_t)quantidade, sizeof(*listas));
    LockSeparado *locks = malloc((size_t)quantidade * sizeof(*locks));
    Resultado r = {0.0, 0, 0, 0};
    long long falhas = 0;

    if (listas == NULL || locks == NULL) {
        free(listas);
        free(locks);
        r.falhas_alocacao = insercoes;
        return r;
    }

    for (int indice = 0; indice < quantidade; indice++) {
        omp_init_lock(&locks[indice].valor);
    }

    double inicio = omp_get_wtime();
    #pragma omp parallel default(none) \
        shared(listas, locks, falhas, insercoes, semente, quantidade)
    {
        #pragma omp single
        {
            #pragma omp taskgroup
            {
                for (long long i = 0; i < insercoes; i++) {
                    #pragma omp task firstprivate(i)
                    {
                        int destino = escolher_lista(i, semente, quantidade);
                        No *novo = malloc(sizeof(*novo));

                        if (novo == NULL) {
                            #pragma omp atomic update
                            falhas++;
                        } else {
                            novo->valor = i;
                            omp_set_lock(&locks[destino].valor);
                            inserir_sem_protecao(&listas[destino], novo);
                            omp_unset_lock(&locks[destino].valor);
                        }
                    }
                }
            }
        }
    }
    r.segundos = omp_get_wtime() - inicio;

    r.falhas_alocacao = falhas;
    r.valido = validar_listas(listas, quantidade,
                              insercoes - falhas, &r.inseridos);

    /* destroy libera recursos do runtime; free libera os vetores dinamicos. */
    for (int indice = 0; indice < quantidade; indice++) {
        omp_destroy_lock(&locks[indice].valor);
    }
    liberar_listas(listas, quantidade);
    free(locks);
    free(listas);
    return r;
}

static int ler_ll(const char *texto, long long minimo, long long *valor)
{
    char *fim;
    long long convertido;

    errno = 0;
    convertido = strtoll(texto, &fim, 10);
    if (errno != 0 || *texto == '\0' || *fim != '\0' || convertido < minimo) {
        return 0;
    }
    *valor = convertido;
    return 1;
}

static void imprimir_resultado(const char *nome, Resultado r)
{
    printf("%-27s %12.6f %12lld %8lld   %s\n",
           nome, r.segundos, r.inseridos, r.falhas_alocacao,
           r.valido ? "OK" : "FALHOU");
}

int main(int argc, char **argv)
{
    long long insercoes = 200000;
    long long threads_lido = 8;
    long long listas_lido = 8;
    long long semente_lida = 2026;

    if (argc > 5 ||
        (argc > 1 && !ler_ll(argv[1], 1, &insercoes)) ||
        (argc > 2 && !ler_ll(argv[2], 1, &threads_lido)) ||
        (argc > 3 && !ler_ll(argv[3], 1, &listas_lido)) ||
        (argc > 4 && !ler_ll(argv[4], 0, &semente_lida)) ||
        threads_lido > INT_MAX || listas_lido > INT_MAX ||
        semente_lida > UINT_MAX) {
        fprintf(stderr,
                "Uso: %s [insercoes>=1] [threads>=1] [listas>=1] [semente]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    int threads = (int)threads_lido;
    int quantidade_listas = (int)listas_lido;
    unsigned int semente = (unsigned int)semente_lida;

    omp_set_dynamic(0);
    omp_set_num_threads(threads);

    printf("Tarefa 9 - insercoes concorrentes em listas encadeadas\n");
    printf("N: %lld | threads: %d | listas na generalizacao: %d | semente: %u\n\n",
           insercoes, threads, quantidade_listas, semente);
    printf("Versao                       Tempo (s)    Inseridos   Falhas   Integridade\n");

    Resultado comum = executar_duas_listas(insercoes, semente, CRITICAL_COMUM);
    Resultado nomeado = executar_duas_listas(insercoes, semente,
                                              CRITICAL_NOMEADO);
    Resultado locks = executar_varias_listas(insercoes, semente,
                                              quantidade_listas);

    imprimir_resultado("2 listas: critical comum", comum);
    imprimir_resultado("2 listas: critical nomeado", nomeado);
    imprimir_resultado("N listas: locks explicitos", locks);

    if (comum.segundos > 0.0 && nomeado.segundos > 0.0) {
        printf("\nRazao comum/nomeado: %.3fx\n",
               comum.segundos / nomeado.segundos);
    }
    return (comum.valido && nomeado.valido && locks.valido)
               ? EXIT_SUCCESS : EXIT_FAILURE;
}
