# Tarefa 14 — ping-pong MPI

`pingpong.c` mede o tempo de troca de mensagens entre dois processos MPI, de 8 bytes
a 4 MB, para identificar onde a latência domina e onde a largura de banda domina.

## Executar

```bash
sbatch job_npad.sh          # no NPAD: mesmo no e nos diferentes, 3 vezes cada

# direto
mpicc -O2 -Wall -Wextra -std=c11 pingpong.c -o pingpong
mpirun -np 2 ./pingpong
```

## Arquivos

| Arquivo | Conteúdo |
|---|---|
| `pingpong.c` | o programa |
| `job_npad.sh` | script Slurm (2 nós exclusivos) |
| `resultados.txt` | saída usada no relatório (CSV) |
| `relatorio/graficos.py` | gera `tempo.svg` e `banda.svg` (mediana das 3 execuções) |
| `relatorio/relatorio.pdf` | relatório com gráficos e código realçado |
| `guia_apresentacao.md` | perguntas para a defesa oral |
