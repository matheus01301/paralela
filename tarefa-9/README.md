# Tarefa 9 — inserções concorrentes em listas encadeadas

O programa cria uma tarefa OpenMP para cada uma das `N` inserções. Cada tarefa
escolhe de forma pseudoaleatória a lista de destino e insere um nó no início.
São comparadas três versões:

1. duas listas protegidas por um único `critical` sem nome;
2. duas listas protegidas por regiões `critical` nomeadas independentes;
3. quantidade de listas definida pelo usuário e um `omp_lock_t` por lista.

Todas as versões percorrem as listas ao final e comparam a quantidade encontrada
com os contadores protegidos, detectando perda de nós por condição de corrida.

## Executar

```powershell
.\executar_testes.ps1 -Insercoes 200000 -Threads 8 -Listas 8 -Execucoes 5
```

Ou diretamente:

```powershell
gcc -O2 -Wall -Wextra -std=c11 -fopenmp listas_insercoes.c -o listas_insercoes.exe
.\listas_insercoes.exe 200000 8 8 2026
```

## Arquivos

| Arquivo | Conteúdo |
|---|---|
| `listas_insercoes.c` | três versões, medição e validação |
| `executar_testes.ps1` | compilação e repetições do experimento |
| `resultados.txt` | resultados usados no relatório |
| `relatorio/relatorio.pdf` | relatório pronto, com código realçado |
| `guia_apresentacao.md` | roteiro para a apresentação e defesa |
