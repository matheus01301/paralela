# Tarefa 10 — mecanismos de sincronização no OpenMP

O estimador de π por Monte Carlo usa `rand_r()` e compara cinco estratégias para
somar os acertos (*hits*):

1. contador compartilhado e `critical` a cada hit;
2. contador compartilhado e `atomic` a cada hit;
3. contador privado e uma soma final com `critical` por thread;
4. contador privado e uma soma final com `atomic` por thread;
5. cláusula `reduction(+:hits)`.

Todas usam as mesmas sementes por thread e `schedule(static)`. O programa verifica
que as cinco versões produzem exatamente o mesmo número de hits.

## Executar

```powershell
.\executar_testes.ps1 -Pontos 20000000 -Threads 8 -Execucoes 5
```

Ou diretamente:

```powershell
gcc -O2 -Wall -Wextra -std=c11 -fopenmp pi_sincronizacao.c -o pi_sincronizacao.exe
.\pi_sincronizacao.exe 20000000 8 2026
```

## Arquivos

| Arquivo | Conteúdo |
|---|---|
| `pi_sincronizacao.c` | implementação e validação das cinco versões |
| `executar_testes.ps1` | compilação e cinco repetições |
| `resultados.txt` | medições usadas no relatório |
| `relatorio/relatorio.pdf` | relatório com o código completo realçado |
| `guia_apresentacao.md` | roteiro para apresentação e defesa |
