# Tarefa 1 — Aproximação de π

A entrega principal calcula uma aproximação de π com a série de Leibniz e mostra
como o número de iterações afeta o erro e o tempo de execução.

## Compilar e executar

```bash
gcc -O2 -Wall -Wextra pi_serie.c -o pi_serie.exe -lm
./pi_serie.exe
```

O programa testa 10⁶, 10⁷, 10⁸ e 10⁹ iterações. No computador usado para o
relatório, o último teste levou 0,8723 segundo e a execução dos cálculos ficou
próxima de um segundo.

## Arquivos principais

| arquivo | conteúdo |
|---|---|
| `pi_serie.c` | código C da tarefa |
| `relatorio/relatorio.pdf` | relatório pronto para apresentação |
| `relatorio/relatorio.md` | texto usado para gerar o PDF |
| `guia_apresentacao.md` | resumo e perguntas para a explicação presencial |

Os demais arquivos da pasta são experimentos adicionais sobre paralelismo e não
fazem parte do relatório desta versão da tarefa.
