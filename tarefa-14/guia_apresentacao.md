# Guia de apresentação — Tarefa 14

## Resumo em 30 segundos

Dois processos MPI trocam a mesma mensagem 1000 vezes, de 8 bytes a 4 MB. Medi com
`MPI_Wtime` no mesmo nó e em nós diferentes do NPAD. Até ~1 KB a **latência**
domina (~1,2 µs na rede). A partir de ~256 KB a **banda** domina (~12 GB/s).

## Números de cor

| | rede | mesmo nó |
|---|---|---|
| latência (8 B) | 1,2 µs | 0,7 µs |
| banda máxima (4 MB) | 12,1 GB/s | ~7,6 GB/s, instável |
| latência domina até | ~1 KB | |
| banda domina a partir de | ~256 KB | |
| ponto de equilíbrio | 1,2 µs × 12,1 GB/s ≈ 14 KB | |

## Se perguntarem

**O que é MPI?** Processos independentes, sem memória compartilhada. Só trocam
dados por mensagens. É o modelo de memória distribuída.

**O que é rank?** O número do processo, dado por `MPI_Comm_rank`. Aqui 0 ou 1.

**Por que o processo 1 começa com `Recv`?** Porque `Send` e `Recv` são
bloqueantes. Se os dois começassem com `Recv`, ficariam esperando um pelo outro:
deadlock.

**Por que 1000 trocas, e não uma?** Uma troca leva ~1 µs, perto da resolução do
relógio. Com 1000, o tempo total fica em milissegundos e eu divido.

**Por que dividir por 2 × 1000?** Cada troca é ida e volta. Quero o tempo de uma
ida.

**Para que a troca de aquecimento?** A primeira mensagem entre dois processos
inclui o custo de preparar a conexão. Não é custo da mensagem.

**O que é latência? E banda?** Latência é o custo fixo de cada mensagem, mesmo
vazia. Banda é quantos bytes por segundo passam depois que a mensagem começou.
Tempo ≈ latência + tamanho / banda.

**Como você viu os regimes no gráfico?** Na latência, a curva de tempo fica quase
deitada: dobrar o tamanho muda o tempo em ×1,0 a ×1,1. Na banda, vira reta: dobrar
o tamanho dobra o tempo (×1,8 a ×1,97).

**Por que no mesmo nó a latência é menor?** A mensagem não passa pela placa de
rede, só pela memória.

**Então por que a rede ganhou nas mensagens grandes?** Dentro do nó quem copia é o
processador, disputando cache e memória. Na rede a placa copia direto da memória
(RDMA), sem usar o processador.

**Qual a diferença entre `MPI_Send`, `MPI_Ssend` e `MPI_Bsend`?** (slide 69)
`Ssend` só retorna quando o outro processo começou a receber (síncrono). `Bsend`
copia para um buffer meu e retorna na hora. `Send` deixa o MPI escolher: mensagem
pequena costuma ir direto para um buffer; grande espera o destino.

**O que isso muda para programas paralelos?** Muitas mensagens pequenas custam
caro, porque cada uma paga a latência. É melhor juntar dados em poucas mensagens
grandes.

## Detalhes do código e do script

**`MPI_CHAR`?** O tipo de cada elemento: 1 byte. Então `tam` elementos = `tam` bytes.

**`MPI_STATUS_IGNORE`?** Não preciso das informações extras da mensagem recebida
(quem mandou, tamanho): eu já sei.

**`calloc`?** Reserva a mensagem já zerada. O conteúdo não importa, só o tamanho.

**`srun --mpi=pmix`?** É como o Slurm inicia os processos MPI no NPAD.

**`-N 1 -n 2` e `-N 2 -n 2 --ntasks-per-node=1`?** Dois processos num nó; e dois
processos, um em cada nó.

**`--cpu-bind=cores`?** Prende cada processo num núcleo, para o sistema não mudar
ele de lugar no meio da medição.

**`export OMPI_MCA_btl=^openib`?** Desliga um componente antigo do Open MPI que só
gerava um aviso. A rede é usada por outro componente (UCX). Os tempos não mudaram.

## Comandos

```bash
ssh npad
cd ~/paralela/tarefa-14
sbatch job_npad.sh

cd tarefa-14/relatorio
python graficos.py && python build_pdf.py
```
