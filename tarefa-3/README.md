# Tarefa 3 — Paralelismo ao nível de instrução

A tarefa compara uma soma acumulativa, que possui dependência entre iterações,
com versões que usam 2, 4 e 8 acumuladores independentes. Também mede o laço de
inicialização e compara os níveis de otimização `-O0`, `-O2` e `-O3`.

## Executar todos os testes

No PowerShell:

```powershell
./executar_testes.ps1
```

O script compila as três versões, executa cada teste três vezes em processos
separados e mostra a mediana do tempo por laço.

Também é possível compilar e executar manualmente:

```bash
gcc -O2 -Wall -Wextra -std=c99 ilp.c -o ilp_O2.exe
./ilp_O2.exe dependente
./ilp_O2.exe multipla4
```

Testes disponíveis: `inicializacao`, `dependente`, `multipla2`, `multipla4` e
`multipla8`.

## Arquivos

| arquivo | conteúdo |
|---|---|
| `ilp.c` | código C com os laços e a medição |
| `executar_testes.ps1` | compilação e coleta das medianas |
| `relatorio/relatorio.pdf` | relatório para entrega |
| `relatorio/relatorio.md` | texto usado para gerar o PDF |
| `guia_apresentacao.md` | resumo e perguntas para a apresentação |

O vetor possui 16 milhões de `double` (aproximadamente 122 MiB). Cada medição é
ajustada automaticamente para durar entre cerca de 0,6 e 1,2 segundo.
