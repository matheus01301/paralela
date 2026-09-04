# Tarefa 7 — tarefas OpenMP em uma lista encadeada

O programa cria uma lista encadeada de nomes de arquivos e compara duas formas
de gerar uma tarefa OpenMP para cada nó:

1. uma versão propositalmente incorreta, na qual todas as threads percorrem a
   lista e criam tarefas repetidas;
2. a versão correta, na qual `single` escolhe uma única thread produtora e
   `firstprivate(atual)` associa cada tarefa ao nó corrente.

## Compilar e executar

```bash
gcc -O2 -Wall -Wextra -std=c99 -fopenmp lista_tarefas.c -o lista_tarefas.exe
./lista_tarefas.exe 4
```

No PowerShell, o script compila, executa três vezes e grava `resultados.txt`:

```powershell
.\executar_testes.ps1 -Threads 4 -Execucoes 3
```

## Arquivos

| Arquivo | Conteúdo |
|---|---|
| `lista_tarefas.c` | lista, versão incorreta, correção e verificação |
| `executar_testes.ps1` | compilação e repetições do experimento |
| `resultados.txt` | resultados medidos na máquina de teste |
| `relatorio/relatorio.pdf` | relatório pronto para entrega, com código realçado |
| `guia_apresentacao.md` | roteiro e perguntas para a defesa |
