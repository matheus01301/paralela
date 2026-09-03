# Tarefa 6 — estimativa estocástica de π com OpenMP

O programa estima π por Monte Carlo e compara quatro versões:

1. sequencial;
2. `parallel for` com uma condição de corrida proposital;
3. contador compartilhado protegido por `critical` em cada acerto;
4. contador `private` por thread e uma agregação final em `critical`.

A versão final separa `#pragma omp parallel` de `#pragma omp for` e demonstra as
cláusulas `private`, `firstprivate`, `lastprivate`, `shared` e `default(none)`.

## Compilar e executar

```bash
gcc -O2 -Wall -Wextra -std=c99 -fopenmp pi_monte_carlo.c -o pi_monte_carlo.exe
./pi_monte_carlo.exe
```

Os argumentos opcionais são quantidade de amostras, número de threads e semente:

```bash
./pi_monte_carlo.exe 10000000 8 2026
```

No PowerShell, o script compila, executa cinco vezes e grava `resultados.txt`:

```powershell
.\executar_testes.ps1
```

## Arquivos

| Arquivo | Conteúdo |
|---|---|
| `pi_monte_carlo.c` | quatro versões e medição do tempo |
| `executar_testes.ps1` | compilação e repetições do experimento |
| `resultados.txt` | resultados medidos na máquina de teste |
| `relatorio/relatorio.pdf` | relatório pronto para entrega, com código realçado |
| `guia_apresentacao.md` | roteiro e perguntas para a defesa |
