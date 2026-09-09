# Tarefa 8 — `rand`, `rand_r` e falso compartilhamento

O programa estima π por Monte Carlo e mede quatro combinações:

1. `rand()` com contador privado e acumulação global em `critical`;
2. `rand()` com um vetor compartilhado de contadores;
3. `rand_r()` com contador privado e `critical`;
4. `rand_r()` com vetor compartilhado.

Na versão em vetor, cada thread possui um índice, mas índices vizinhos ficam na
mesma linha de cache. Os incrementos são mantidos como acessos à memória com
`volatile`, tornando observável o falso compartilhamento solicitado no estudo.

## Executar o experimento

O benchmark usa Linux/WSL porque `rand_r()` é uma extensão POSIX e não faz parte
da biblioteca C padrão nem da UCRT do Windows.

```powershell
.\executar_testes.ps1 -Pontos 10000000 -Threads 16 -Execucoes 5
```

Ou diretamente em Linux:

```bash
gcc -O2 -Wall -Wextra -std=c11 -fopenmp pi_rand_openmp.c -o pi_rand_openmp
./pi_rand_openmp 10000000 16 2026
```

## Arquivos

| Arquivo | Conteúdo |
|---|---|
| `pi_rand_openmp.c` | implementação das quatro versões |
| `executar_testes.ps1` | compilação e cinco repetições do benchmark |
| `resultados.txt` | medições usadas no relatório |
| `relatorio/relatorio.pdf` | relatório pronto para entrega, com código realçado |
| `guia_apresentacao.md` | roteiro curto para apresentação e defesa |
