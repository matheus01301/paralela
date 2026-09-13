# Guia de apresentação — Tarefa 10

## Resumo em 30 segundos

Comparei cinco maneiras de somar os hits do Monte Carlo usando `rand_r`. Um
contador compartilhado gera aproximadamente 15,7 milhões de sincronizações:
`atomic` foi 5,7 vezes mais rápido que `critical`, mas ainda há contenção. Com um
contador privado, ocorre apenas uma combinação por thread, ou oito no teste, e o
tempo caiu aproximadamente 57 vezes. `reduction` automatiza essa privatização e é a solução
mais clara para uma soma.

## Ordem para mostrar o código

1. `rand_r(&estado)` demonstra que cada thread possui estado aleatório privado.
2. Mostrar o incremento compartilhado com `critical`.
3. Trocar apenas a diretiva por `atomic update` e explicar sua restrição.
4. Mostrar `hits_privados` e a única combinação final por thread.
5. Encerrar com `reduction(+:hits)`, que descreve o padrão automaticamente.

## Perguntas prováveis

**Por que `atomic` foi mais rápido que `critical`?**

Porque protege uma forma restrita de acesso e normalmente vira uma instrução ou
sequência especializada. `critical` implementa exclusão mútua para um bloco
arbitrário e possui um protocolo mais geral.

**É possível colocar qualquer linha depois de `atomic`?**

Não. A diretiva precisa estar associada a uma forma aceita pelo OpenMP, como
`x++` ou `x += valor`. Ela não protege livremente várias instruções ou um
invariante entre campos.

**Por que `atomic` ainda é lento no contador compartilhado?**

Todas as threads modificam a mesma linha de cache. As operações continuam
ordenadas pela coerência e há uma atualização para cada hit.

**Quantos atomics existem na versão com contador privado?**

Um por thread. No teste foram oito, enquanto a versão compartilhada realizou
cerca de 15,7 milhões. Essa redução de frequência explica o maior ganho.

**Por que `critical` e `atomic` privados tiveram o mesmo tempo?**

Porque ambos foram executados apenas oito vezes. O custo dominante passou a ser
a geração e a classificação dos 20 milhões de pontos.

**A redução não sincroniza?**

Ela combina resultados internamente. O programa mostra zero atualizações
explicitamente sincronizadas porque o fonte não contém `critical` nem `atomic`.

**Quando usar `critical` nomeado?**

Quando há poucos recursos independentes conhecidos ao compilar. Nomes diferentes
evitam que a proteção de um recurso bloqueie outro.

**Quando usar locks explícitos?**

Quando a quantidade de recursos é dinâmica, o lock precisa ser escolhido por
índice ou é necessário controlar aquisição e liberação. O preço é mais código,
risco de deadlock e necessidade de cuidar do ciclo de vida e falso
compartilhamento.

## Roteiro para decorar

1. Redução suportada → `reduction`.
2. Dá para acumular localmente → privatizar e combinar raramente.
3. Uma atualização simples → `atomic`.
4. Um bloco ou invariante composto → `critical`.
5. Recursos fixos independentes → `critical` nomeado.
6. Recursos dinâmicos ou controle avançado → locks explícitos.

## Comandos

```powershell
cd tarefa-10
.\executar_testes.ps1 -Pontos 20000000 -Threads 8 -Execucoes 5
cd relatorio
python build_pdf.py
```
