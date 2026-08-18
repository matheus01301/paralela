/* ============================================================================
 * mxv.c - Multiplicacao matriz-vetor: acesso por linhas x acesso por colunas
 *
 * Calcula y = A*x de duas formas matematicamente identicas, trocando apenas a
 * ORDEM DOS LACOS. As duas leem exatamente os mesmos N*N elementos e fazem
 * exatamente as mesmas 2*N*N operacoes de ponto flutuante. A unica diferenca
 * e' o PADRAO DE ACESSO A' MEMORIA.
 *
 * Em C, uma matriz e' armazenada por LINHAS (row-major): A[i][j] e A[i][j+1]
 * sao vizinhos na memoria; A[i][j] e A[i+1][j] estao separados por N*8 bytes.
 *
 *   POR LINHAS  : laco interno varia j -> percorre a memoria sequencialmente
 *   POR COLUNAS : laco interno varia i -> salta de N*8 em N*8 bytes
 *
 * Compilar: gcc -O2 -Wall -Wextra -std=c99 mxv.c -o mxv.exe -lm
 * Executar: ./mxv.exe            (N de 64 a 8192)
 *           ./mxv.exe 2048       (para no N indicado)
 * ==========================================================================*/

#define _POSIX_C_SOURCE 199309L /* libera clock_gettime e CLOCK_MONOTONIC */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Caches desta maquina (i7-13650HX), para anotar a tabela. Ajuste se rodar
 * em outro processador. */
#define L1D_BYTES (48 * 1024)
#define L2_BYTES (1280 * 1024)
#define L3_BYTES (24 * 1024 * 1024)
#define LINHA_CACHE 64 /* bytes por linha de cache = 8 doubles */

/* ---------------------------------------------------------------------------
 * MEDICAO DE TEMPO
 *
 * clock_gettime(CLOCK_MONOTONIC) e' o relogio monotonico do sistema, com
 * resolucao de nanossegundos. Monotonico significa que ele nunca anda para
 * tras - diferente do CLOCK_REALTIME, que pode saltar quando o NTP acerta o
 * horario da maquina e produziria intervalos negativos.
 *
 * Tambem nao usamos clock(): ele mede tempo de CPU, nao tempo de parede. Para
 * medir o que o usuario espera - e para comparar com versoes paralelas depois -
 * o tempo de parede e' o correto.
 * -------------------------------------------------------------------------*/
static double agora(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* ---------------------------------------------------------------------------
 * VERSAO 1 - ACESSO POR LINHAS (laco interno varia a COLUNA j)
 *
 *   for i:              <- percorre as linhas
 *     for j:            <- laco interno: caminha ao longo de UMA linha
 *       soma += A[i][j] * x[j]
 *
 * A[i*n + j] com j crescendo de 1 em 1 le' a memoria sequencialmente. Cada
 * linha de cache de 64 bytes trazida da memoria contem 8 doubles, e os 8 sao
 * usados. Alem disso o prefetcher do processador reconhece o padrao sequencial
 * e busca as proximas linhas antes de serem pedidas.
 *
 * Bonus desta ordem: como i e' fixo no laco interno, a soma parcial cabe num
 * REGISTRADOR (a variavel `soma`) e y[i] e' escrito uma unica vez por linha.
 * -------------------------------------------------------------------------*/
/* `ld` (leading dimension) = distancia em elementos entre o inicio de duas
 * linhas consecutivas. Normalmente ld == n, mas o modo "conflito" usa
 * ld = n+1 para quebrar o alinhamento em potencia de 2. */
static void mxv_linhas(const double *A, const double *x, double *y, int n,
                       size_t ld)
{
    for (int i = 0; i < n; i++) {
        double soma = 0.0;
        const double *linha = A + (size_t)i * ld;
        for (int j = 0; j < n; j++) {
            soma += linha[j] * x[j];
        }
        y[i] = soma;
    }
}

/* ---------------------------------------------------------------------------
 * VERSAO 2 - ACESSO POR COLUNAS (laco interno varia a LINHA i)
 *
 *   for j:              <- percorre as colunas
 *     for i:            <- laco interno: desce UMA coluna
 *       y[i] += A[i][j] * x[j]
 *
 * A[i*n + j] com i crescendo de 1 em 1 salta n*8 bytes a cada passo. Cada
 * acesso cai numa linha de cache DIFERENTE: dos 8 doubles trazidos, apenas 1 e'
 * usado agora. Os outros 7 so' serao uteis quando o laco chegar nas colunas
 * j+1..j+7 - o que so' acontece depois de percorrer a coluna inteira.
 *
 * Se a matriz couber no cache, aquelas linhas ainda estarao la' e o desperdicio
 * nao aparece. Se nao couber, elas terao sido despejadas, e a mesma linha de
 * cache sera' buscada da memoria 8 vezes - uma por coluna.
 *
 * Note tambem que y[i] agora e' lido e escrito na memoria N*N vezes, em vez de
 * ficar num registrador. Como y tem apenas n doubles, ele costuma caber no L1,
 * entao esse nao e' o efeito dominante - mas soma.
 * -------------------------------------------------------------------------*/
static void mxv_colunas(const double *A, const double *x, double *y, int n,
                        size_t ld)
{
    for (int i = 0; i < n; i++) {
        y[i] = 0.0;
    }
    for (int j = 0; j < n; j++) {
        const double xj = x[j];
        for (int i = 0; i < n; i++) {
            y[i] += A[(size_t)i * ld + j] * xj;
        }
    }
}

/* ---------------------------------------------------------------------------
 * APOIO
 * -------------------------------------------------------------------------*/

/* Onde a matriz cabe, para anotar a tabela. */
static const char *onde_cabe(double bytes)
{
    if (bytes <= L1D_BYTES) return "L1";
    if (bytes <= L2_BYTES) return "L2";
    if (bytes <= L3_BYTES) return "L3";
    return "RAM";
}

/* Repeticoes suficientes para cada medicao durar o bastante para o relogio
 * ter resolucao, sem tornar os N grandes intoleraveis. */
static int repeticoes(int n)
{
    double flops = 2.0 * (double)n * (double)n;
    double alvo = 2.0e8; /* ~0,2 s a 1 GFLOP/s */
    int r = (int)(alvo / flops);
    if (r < 3) r = 3;
    if (r > 5000) r = 5000;
    return r;
}

static double mediana(double *v, int n)
{
    /* insercao: n e' minusculo */
    for (int i = 1; i < n; i++) {
        double k = v[i];
        int j = i - 1;
        while (j >= 0 && v[j] > k) {
            v[j + 1] = v[j];
            j--;
        }
        v[j + 1] = k;
    }
    return v[n / 2];
}

/* Mede as duas versoes para um N e um `ld` dados. Devolve os tempos medianos
 * por chamada e se os dois resultados bateram bit a bit. Retorna 0 se faltou
 * memoria. */
static int medir(int n, size_t ld, double *t_linhas, double *t_colunas,
                 int *iguais)
{
    size_t elems = (size_t)n * ld;
    double *A = malloc(elems * sizeof(double));
    double *x = malloc((size_t)n * sizeof(double));
    double *y1 = malloc((size_t)n * sizeof(double));
    double *y2 = malloc((size_t)n * sizeof(double));

    if (!A || !x || !y1 || !y2) {
        free(A); free(x); free(y1); free(y2);
        return 0;
    }

    /* Dados deterministicos e sem denormais/NaN, para o tempo depender do
     * padrao de acesso e nao do valor dos numeros. */
    for (size_t k = 0; k < elems; k++) {
        A[k] = (double)((k % 17) + 1) * 0.5;
    }
    for (int k = 0; k < n; k++) {
        x[k] = (double)((k % 7) + 1) * 0.25;
    }

    int reps = repeticoes(n);
    double tempos1[16], tempos2[16];
    int amostras = reps > 5 ? 5 : 3;

    /* Aquecimento: traz as paginas para a RAM (first touch) e enche os caches,
     * para nao medir falha de pagina no lugar de falha de cache. */
    mxv_linhas(A, x, y1, n, ld);
    mxv_colunas(A, x, y2, n, ld);

    for (int s = 0; s < amostras; s++) {
        double t0 = agora();
        for (int r = 0; r < reps; r++) mxv_linhas(A, x, y1, n, ld);
        tempos1[s] = (agora() - t0) / reps;

        t0 = agora();
        for (int r = 0; r < reps; r++) mxv_colunas(A, x, y2, n, ld);
        tempos2[s] = (agora() - t0) / reps;
    }

    *t_linhas = mediana(tempos1, amostras);
    *t_colunas = mediana(tempos2, amostras);
    /* As duas versoes somam sobre j na mesma ordem, entao os resultados devem
     * bater BIT A BIT. Se nao baterem, ha' bug - nao arredondamento. */
    *iguais = (memcmp(y1, y2, (size_t)n * sizeof(double)) == 0);

    free(A); free(x); free(y1); free(y2);
    return 1;
}

/* Teste do efeito de conflito de cache: mesmo N, mudando so' a distancia entre
 * linhas consecutivas. Com ld = N (potencia de 2), elementos de uma mesma
 * coluna caem todos no mesmo conjunto do cache e se despejam mutuamente. Com
 * ld = N+1, cada linha desloca o endereco em 8 bytes e a coluna se espalha por
 * conjuntos diferentes. Se o tempo despencar, a causa era conflito - nao falta
 * de capacidade. */
static void modo_conflito(void)
{
    int casos[] = {512, 1024, 2048, 4096};
    printf("Conflito de conjunto: mesmo N, mudando so' a distancia entre linhas\n\n");
    printf("  %6s  %14s  %14s  %10s  %14s  %14s  %10s  %8s\n", "N",
           "COLUNAS ld=N", "COLUNAS ld=N+1", "GANHO", "LINHAS ld=N",
           "LINHAS ld=N+1", "GANHO", "RAZAO+1");
    printf("  --------------------------------------------------------------"
           "---------------------------------------\n");

    for (int i = 0; i < (int)(sizeof(casos) / sizeof(casos[0])); i++) {
        int n = casos[i];
        double l0, c0, l1, c1;
        int eq0, eq1;

        if (!medir(n, (size_t)n, &l0, &c0, &eq0)) break;
        if (!medir(n, (size_t)n + 1, &l1, &c1, &eq1)) break;

        printf("  %6d  %13.6fs  %13.6fs  %9.2fx  %13.6fs  %13.6fs  %9.2fx  "
               "%7.2fx%s\n",
               n, c0, c1, c0 / c1, l0, l1, l0 / l1, c1 / l1,
               (eq0 && eq1) ? "" : "  <-- RESULTADOS DIFEREM!");
        fflush(stdout);
    }

    printf("\n  GANHO   = quanto o padding de 1 elemento por linha acelerou\n");
    printf("  RAZAO+1 = colunas/linhas ja' com o padding, isolando o efeito de\n");
    printf("            capacidade do efeito de conflito.\n");
}

int main(int argc, char *argv[])
{
    /* Varredura padrao: potencias de 2, para ver o quadro geral. */
    int largos[] = {64, 128, 256, 512, 1024, 2048, 4096, 8192};
    /* Varredura fina: passos menores em volta das fronteiras de L2 (~N=404) e
     * de L3 (~N=1774), para localizar onde exatamente os tempos divergem. */
    int finos[] = {256, 320, 384, 416, 448, 480, 512, 576,  640,
                   768, 1024, 1280, 1536, 1664, 1792, 1920, 2048, 2304};

    int *tamanhos = largos;
    int n_tamanhos = (int)(sizeof(largos) / sizeof(largos[0]));
    int limite = 0;

    if (argc > 1 && strcmp(argv[1], "conflito") == 0) {
        modo_conflito();
        return 0;
    } else if (argc > 1 && strcmp(argv[1], "fino") == 0) {
        tamanhos = finos;
        n_tamanhos = (int)(sizeof(finos) / sizeof(finos[0]));
    } else if (argc > 1) {
        limite = atoi(argv[1]);
    }

    printf("Multiplicacao matriz-vetor: y = A*x  (%d bytes por double)\n",
           (int)sizeof(double));
    printf("Linha de cache: %d bytes = %d doubles\n", LINHA_CACHE,
           LINHA_CACHE / (int)sizeof(double));
    printf("Caches: L1d %d KB | L2 %d KB | L3 %d MB\n\n", L1D_BYTES / 1024,
           L2_BYTES / 1024, L3_BYTES / (1024 * 1024));

    printf("  %6s  %10s  %5s  %5s  %12s  %12s  %8s  %9s  %9s\n", "N",
           "MATRIZ", "CABE", "REPS", "LINHAS(s)", "COLUNAS(s)", "RAZAO",
           "MFLOP/s L", "MFLOP/s C");
    printf("  ----------------------------------------------------------------"
           "-------------------------------\n");

    for (int t = 0; t < n_tamanhos; t++) {
        int n = tamanhos[t];
        if (limite > 0 && n > limite) break;

        size_t elems = (size_t)n * (size_t)n;
        double bytes = (double)elems * sizeof(double);
        double t1, t2;
        int iguais;

        if (!medir(n, (size_t)n, &t1, &t2, &iguais)) {
            fprintf(stderr, "sem memoria para N=%d (%.0f MB)\n", n,
                    bytes / (1024.0 * 1024.0));
            break;
        }

        double flops = 2.0 * (double)elems;
        printf("  %6d  %8.1f MB  %5s  %5d  %12.6f  %12.6f  %7.2fx  %9.0f  "
               "%9.0f%s\n",
               n, bytes / (1024.0 * 1024.0), onde_cabe(bytes), repeticoes(n),
               t1, t2, t2 / t1, flops / t1 / 1e6, flops / t2 / 1e6,
               iguais ? "" : "  <-- RESULTADOS DIFEREM!");
        fflush(stdout);
    }

    printf("\n  RAZAO = tempo(colunas) / tempo(linhas). 1.00x significa empate.\n");
    printf("  As duas versoes fazem as MESMAS 2*N*N operacoes sobre os MESMOS\n");
    printf("  dados. Toda diferenca vem do padrao de acesso a' memoria.\n");

    return 0;
}
