# Tarefa 11 — difusão viscosa (Navier-Stokes sem pressão) com OpenMP

Simulação do campo de velocidade de um fluido considerando **somente a
viscosidade**. Sem advecção, sem pressão, sem forças externas, cada componente
da velocidade obedece a uma equação de difusão:

```
du/dt = nu * (d2u/dx2 + d2u/dy2)
dv/dt = nu * (d2v/dx2 + d2v/dy2)
```

- Malha regular `n x n` sobre o domínio `[0,1] x [0,1]`.
- Diferenças centrais no espaço, Euler explícito no tempo.
- Bordas periódicas (um campo constante continua constante também nas bordas).
- Dois estados em memória (`atual` e `novo`), com troca de ponteiros a cada passo.

O programa executa quatro validações sequenciais e depois compara sete versões
do laço: sequencial e as combinações de `schedule(static|dynamic|guided)` com e
sem `collapse(2)`. Todas as versões paralelas têm o resultado comparado byte a
byte com o sequencial.

## Executar

No NPAD (Slurm):

```bash
sbatch job_npad.sh
cat slurm-<JobID>.out
```

Direto:

```bash
gcc -O2 -Wall -Wextra -std=c11 -fopenmp difusao_viscosa.c -o difusao_viscosa -lm
./difusao_viscosa 1024 200 32 5      # n, passos, threads, execucoes
```

## Arquivos

| Arquivo | Conteúdo |
|---|---|
| `difusao_viscosa.c` | simulação, validações e comparação de `schedule` / `collapse` |
| `job_npad.sh` | script Slurm usado no NPAD (nó exclusivo, threads fixadas nos núcleos) |
| `resultados.txt` | saída usada no relatório |
| `relatorio/relatorio.pdf` | relatório com o código completo realçado |
| `guia_apresentacao.md` | roteiro para a defesa oral |
