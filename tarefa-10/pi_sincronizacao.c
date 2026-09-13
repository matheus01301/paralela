/*
 * Tarefa 10 - critical, atomic, contadores privados e reduction
 *
 * Compilar:
 *   gcc -O2 -Wall -Wextra -std=c11 -fopenmp pi_sincronizacao.c -o pi_sincronizacao.exe
 *
 * Executar:
 *   pi_sincronizacao.exe [pontos] [threads] [semente]
 */

#define _POSIX_C_SOURCE 199506L

#include <errno.h>
#include <limits.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

/* rand_r nao existe na UCRT/MinGW. No Windows, esta funcao compativel permite
 * executar o estudo; no Linux, e usada a implementacao POSIX da libc. */
#ifdef _WIN32
static int rand_r(unsigned int *semente)
{
    unsigned int proximo = *semente;
    unsigned int resultado;

    proximo = proximo * 1103515245u + 12345u;
    resultado = (proximo / 65536u) % 2048u;
    proximo = proximo * 1103515245u + 12345u;
    resultado = (resultado << 10) ^ ((proximo / 65536u) % 1024u);
    proximo = proximo * 1103515245u + 12345u;
    resultado = (resultado << 10) ^ ((proximo / 65536u) % 1024u);
    *semente = proximo;
    return (int)(resultado % ((unsigned int)RAND_MAX + 1u));
}
#endif

typedef struct {
    const char *nome;
    long long hits;
    long long atualizacoes_explicitas;
    double pi;
    double segundos;
} Resultado;

static long long hits_compartilhados;

static unsigned int semente_da_thread(unsigned int base, int id)
{
    return base + 0x9e3779b9u * (unsigned int)(id + 1);
}

static int ponto_dentro_do_circulo(unsigned int *estado)
{
    double x = (double)rand_r(estado) / (double)RAND_MAX;
    double y = (double)rand_r(estado) / (double)RAND_MAX;
    return x * x + y * y <= 1.0;
}

static Resultado finalizar(const char *nome, long long hits,
                           long long sincronizacoes, long long pontos,
                           double inicio)
{
    Resultado r;
    r.nome = nome;
    r.hits = hits;
    r.atualizacoes_explicitas = sincronizacoes;
    r.pi = 4.0 * (double)hits / (double)pontos;
    r.segundos = omp_get_wtime() - inicio;
    return r;
}

/* 1) Cada hit atualiza imediatamente o contador compartilhado sob critical. */
static Resultado compartilhado_critical(long long pontos,
                                         unsigned int semente)
{
    double inicio = omp_get_wtime();
    hits_compartilhados = 0;

    #pragma omp parallel default(none) \
        shared(pontos, semente, hits_compartilhados)
    {
        int id = omp_get_thread_num();
        unsigned int estado = semente_da_thread(semente, id);

        #pragma omp for schedule(static)
        for (long long i = 0; i < pontos; i++) {
            if (ponto_dentro_do_circulo(&estado)) {
                #pragma omp critical
                {
                    hits_compartilhados++;
                }
            }
        }
    }

    return finalizar("compartilhado + critical", hits_compartilhados,
                     hits_compartilhados, pontos, inicio);
}

/* 2) Cada hit atualiza imediatamente o contador com uma operacao atomica. */
static Resultado compartilhado_atomic(long long pontos,
                                       unsigned int semente)
{
    double inicio = omp_get_wtime();
    hits_compartilhados = 0;

    #pragma omp parallel default(none) \
        shared(pontos, semente, hits_compartilhados)
    {
        int id = omp_get_thread_num();
        unsigned int estado = semente_da_thread(semente, id);

        #pragma omp for schedule(static)
        for (long long i = 0; i < pontos; i++) {
            if (ponto_dentro_do_circulo(&estado)) {
                #pragma omp atomic update
                hits_compartilhados++;
            }
        }
    }

    return finalizar("compartilhado + atomic", hits_compartilhados,
                     hits_compartilhados, pontos, inicio);
}

/* 3) Cada thread conta localmente e entra no critical somente uma vez. */
static Resultado privado_critical(long long pontos, unsigned int semente,
                                  int threads)
{
    double inicio = omp_get_wtime();
    hits_compartilhados = 0;

    #pragma omp parallel default(none) \
        shared(pontos, semente, hits_compartilhados)
    {
        int id = omp_get_thread_num();
        unsigned int estado = semente_da_thread(semente, id);
        long long hits_privados = 0;

        #pragma omp for schedule(static)
        for (long long i = 0; i < pontos; i++) {
            hits_privados += ponto_dentro_do_circulo(&estado);
        }

        #pragma omp critical
        {
            hits_compartilhados += hits_privados;
        }
    }

    return finalizar("privado + critical", hits_compartilhados,
                     threads, pontos, inicio);
}

/* 4) Cada thread conta localmente e faz somente uma soma atomica no final. */
static Resultado privado_atomic(long long pontos, unsigned int semente,
                                int threads)
{
    double inicio = omp_get_wtime();
    hits_compartilhados = 0;

    #pragma omp parallel default(none) \
        shared(pontos, semente, hits_compartilhados)
    {
        int id = omp_get_thread_num();
        unsigned int estado = semente_da_thread(semente, id);
        long long hits_privados = 0;

        #pragma omp for schedule(static)
        for (long long i = 0; i < pontos; i++) {
            hits_privados += ponto_dentro_do_circulo(&estado);
        }

        #pragma omp atomic update
        hits_compartilhados += hits_privados;
    }

    return finalizar("privado + atomic", hits_compartilhados,
                     threads, pontos, inicio);
}

/* 5) O OpenMP cria acumuladores privados e os combina automaticamente. */
static Resultado com_reduction(long long pontos, unsigned int semente)
{
    long long hits = 0;
    double inicio = omp_get_wtime();

    #pragma omp parallel default(none) shared(pontos, semente) \
        reduction(+:hits)
    {
        int id = omp_get_thread_num();
        unsigned int estado = semente_da_thread(semente, id);

        #pragma omp for schedule(static)
        for (long long i = 0; i < pontos; i++) {
            hits += ponto_dentro_do_circulo(&estado);
        }
    }

    return finalizar("reduction", hits, 0, pontos, inicio);
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

static void imprimir(Resultado r)
{
    printf("%-25s %12lld   %.9f %12lld %11.6f\n",
           r.nome, r.hits, r.pi, r.atualizacoes_explicitas, r.segundos);
}

int main(int argc, char **argv)
{
    long long pontos = 20000000;
    long long threads_lido = 8;
    long long semente_lida = 2026;
    Resultado resultados[5];

    if (argc > 4 ||
        (argc > 1 && !ler_ll(argv[1], 1, &pontos)) ||
        (argc > 2 && !ler_ll(argv[2], 1, &threads_lido)) ||
        (argc > 3 && !ler_ll(argv[3], 0, &semente_lida)) ||
        threads_lido > INT_MAX || semente_lida > UINT_MAX) {
        fprintf(stderr, "Uso: %s [pontos>=1] [threads>=1] [semente]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    int threads = (int)threads_lido;
    unsigned int semente = (unsigned int)semente_lida;
    int aquecimento = 0;

    omp_set_dynamic(0);
    omp_set_num_threads(threads);

    /* Cria a equipe antes das medicoes para reduzir o efeito de inicializacao. */
    #pragma omp parallel reduction(+:aquecimento)
    aquecimento += omp_get_thread_num() >= 0;
    if (aquecimento != threads) {
        threads = aquecimento;
    }

    resultados[0] = compartilhado_critical(pontos, semente);
    resultados[1] = compartilhado_atomic(pontos, semente);
    resultados[2] = privado_critical(pontos, semente, threads);
    resultados[3] = privado_atomic(pontos, semente, threads);
    resultados[4] = com_reduction(pontos, semente);

    printf("Tarefa 10 - sincronizacao da soma de hits com rand_r\n");
    printf("Pontos: %lld | threads: %d | semente: %u\n\n",
           pontos, threads, semente);
    printf("Versao                            Hits          pi  Atualiz. explic. Tempo (s)\n");
    for (int i = 0; i < 5; i++) {
        imprimir(resultados[i]);
    }

    int valido = 1;
    for (int i = 1; i < 5; i++) {
        if (resultados[i].hits != resultados[0].hits) {
            valido = 0;
        }
    }
    printf("\nValidacao (mesmo numero de hits): %s\n",
           valido ? "OK" : "FALHOU");
    return valido ? EXIT_SUCCESS : EXIT_FAILURE;
}
