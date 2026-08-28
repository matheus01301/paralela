# Tarefa 5 — contagem de números primos com OpenMP

O programa conta quantos números primos existem entre 2 e um limite `n`. Ele
executa primeiro o laço sequencial e depois o mesmo laço com a diretiva
`#pragma omp parallel for`, comparando o resultado e o tempo das duas versões.

A variável `quantidade` é compartilhada na versão paralela. Como várias threads
podem executar `quantidade++` ao mesmo tempo, há uma condição de corrida e o
resultado paralelo pode ser diferente do sequencial. Isso foi mantido de
propósito para demonstrar o desafio de correção pedido na atividade.

## Compilar e executar

```bash
gcc -O2 -Wall -Wextra -std=c99 -fopenmp primos.c -o primos.exe
./primos.exe
```

O limite e o número de threads podem ser informados como argumentos:

```bash
./primos.exe 10000000 8
```

No PowerShell, o script compila e executa o programa cinco vezes:

```powershell
.\executar_testes.ps1
```

As repetições ajudam a mostrar que uma condição de corrida é não determinística:
o erro pode mudar ou até não ser percebido em determinada execução. O arquivo
`resultados.txt` guarda a saída observada na máquina de teste.

## Os dois desafios observados

- **Correção:** `quantidade++` é uma operação de leitura, soma e escrita, não uma
  operação atômica. Incrementos podem ser perdidos quando as threads acessam o
  contador simultaneamente. Uma solução seria usar `reduction(+:quantidade)`, mas
  ela não foi aplicada ao laço principal porque a atividade pede para observar e
  explicar o problema.
- **Distribuição de carga:** as iterações são divididas entre as threads, mas
  testar números maiores tende a exigir mais divisões. Além disso, números
  compostos podem ser descartados rapidamente, enquanto um primo precisa ser
  testado até sua raiz quadrada. Algumas threads podem terminar antes das outras.

Esses efeitos podem não ficar evidentes em toda execução. A ausência de diferença
ou de uma grande perda de desempenho não significa que o problema conceitual não
exista.

## Arquivos

| arquivo | conteúdo |
|---|---|
| `primos.c` | versões sequencial e paralela e medição do tempo |
| `executar_testes.ps1` | compilação e cinco execuções do experimento |
| `resultados.txt` | resultados medidos na máquina de teste |
| `relatorio/relatorio.pdf` | relatório pronto para entrega |
| `guia_apresentacao.md` | resumo e perguntas para a apresentação |
