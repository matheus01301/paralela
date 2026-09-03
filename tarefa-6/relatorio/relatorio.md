# Estimativa estocástica de π com OpenMP

## 1. Objetivo e método

O objetivo foi estimar π por Monte Carlo, paralelizar o cálculo com OpenMP,
observar uma condição de corrida e corrigi-la com `critical`. Também foram
testadas as cláusulas `private`, `firstprivate`, `lastprivate`, `shared` e
`default(none)`.

São gerados pontos uniformes no quadrado `[-1,1] × [-1,1]`. Um ponto pertence ao
círculo unitário quando `x² + y² ≤ 1`. Como a área do círculo é π e a área do
quadrado é 4, a estimativa é:

<div class="formula">π̂ = 4 · pontos dentro do círculo / total de pontos</div>

Cada ponto é gerado a partir da semente e do índice da amostra. Assim, não existe
um estado aleatório global sendo alterado pelas threads, e todas as versões usam
os mesmos pontos. Mesmo quando o programa está correto, o resultado é apenas uma
estimativa; em geral, o erro estatístico diminui quando o número de amostras
aumenta.

## 2. Condição de corrida

A primeira versão paralela apenas acrescenta a diretiva solicitada:

```c
#pragma omp parallel for
for (long long i = 0; i < amostras; i++) {
    /* geração e teste do ponto */
    if (x * x + y * y <= 1.0) {
        dentro++;
    }
}
```

`parallel` cria a equipe de threads e `for` distribui as iterações entre elas. A
forma combinada `parallel for` executa essas duas ações em uma única diretiva.

O índice do laço e as variáveis declaradas dentro dele são privados, mas `dentro`
é compartilhado. `dentro++` envolve leitura, soma e escrita. Duas threads podem
ler o mesmo valor e gravar o mesmo sucessor, perdendo um incremento. Portanto, o
resultado depende da ordem imprevisível dos acessos: isso é uma condição de
corrida. Obter o resultado correto em uma execução, por exemplo com uma thread,
não prova que a versão paralela seja segura.

Mais amostras melhoram a estimativa correta, mas não corrigem essa falha. Na
versão com corrida, a quantidade de incrementos perdidos pode variar entre as
execuções e não segue uma regra determinística.

## 3. Correção com `critical`

A correção direta coloca cada `dentro++` em uma região `critical`. Apenas uma
thread por vez pode executar esse bloco, como se ele estivesse protegido por uma
trava. A contagem fica correta, porém milhões de entradas na região crítica
serializam grande parte do programa.

A versão final separa `parallel` de `for`. Isso permite criar um contador por
thread antes do laço e somá-lo ao total depois:

```c
#pragma omp parallel default(none) \
    shared(amostras, dentro, final_x, final_y) \
    firstprivate(semente) private(i, x, y, dentro_local)
{
    dentro_local = 0;

    #pragma omp for lastprivate(final_x, final_y)
    for (i = 0; i < amostras; i++) {
        gerar_ponto(semente, i, &x, &y);
        final_x = x; final_y = y;
        dentro_local += (x * x + y * y <= 1.0);
    }

    #pragma omp critical
    {
        dentro += dentro_local;
    }
}
```

Cada thread altera apenas seu `dentro_local` e entra no `critical` uma única vez.
Com 20 threads, são 20 atualizações protegidas, em vez de aproximadamente 7,85
milhões.

Sem a exigência de usar `critical`, uma soma como esta seria normalmente escrita
com `reduction(+:dentro)`. `atomic` também poderia proteger cada incremento.

## 4. Explicação rápida dos conceitos

**Condição de corrida:** acontece quando duas ou mais threads acessam o mesmo dado
ao mesmo tempo, pelo menos uma delas escreve e não existe sincronização. Neste
programa, a corrida está em `dentro++`: threads podem ler o mesmo valor e um dos
incrementos acaba perdido.

**`critical`:** faz somente uma thread por vez executar o bloco protegido. Ele
corrige a atualização de `dentro`, mas pode deixar o programa lento quando usado
dentro de todas as iterações. Na versão final, cada thread entra nele apenas uma
vez para somar seu resultado local.

**`shared`:** todas as threads acessam a mesma instância da variável. `amostras`
pode ser compartilhada com segurança porque é apenas lida. `dentro` também é
compartilhada, mas suas escritas precisam do `critical`.

**`private`:** cria uma cópia independente para cada thread, sem copiar o valor
que existia antes. Por isso cada thread possui seu próprio `dentro_local` e deve
inicializá-lo com zero. `i`, `x` e `y` também são privados para que uma thread não
altere os valores usados por outra.

**`firstprivate`:** funciona como `private`, mas copia o valor inicial para cada
thread. Assim, todas recebem uma cópia da semente 2026. Se fosse usado somente
`private(semente)`, seria necessário inicializá-la dentro da região antes do uso.

**`lastprivate`:** ao terminar o `for`, copia para a variável externa o valor da
última iteração na ordem normal do laço. Aqui, `final_x` e `final_y` recebem as
coordenadas da amostra `amostras-1`. Não significa a thread que terminou por
último, e essas coordenadas não participam da estimativa de π; elas apenas
demonstram a cláusula.

**`default(none)`:** obriga o programador a declarar explicitamente o escopo das
variáveis externas usadas na região paralela. Isso ajuda o compilador a encontrar
variáveis esquecidas e torna o código mais claro. Ele não corrige corridas
automaticamente: uma variável `shared` ainda precisa de sincronização quando há
escritas concorrentes.

Em resumo, `shared` mantém um dado comum; `private` cria cópias sem valor inicial;
`firstprivate` cria cópias com valor inicial; `lastprivate` copia para fora o
resultado da última iteração; e `critical` protege uma operação compartilhada.

## 5. Testes e resultados

O programa foi compilado com:

```text
gcc -O2 -Wall -Wextra -std=c99 -fopenmp pi_monte_carlo.c -o pi_monte_carlo.exe
```

Foram realizadas cinco execuções com 10.000.000 de amostras, semente 2026 e 20
threads no Intel Core i7-13650HX. O tempo de parede foi medido por
`omp_get_wtime()`.

| Combinação testada | Resultado | Mediana do tempo |
|---|---:|---:|
| Sequencial | π = 3,141793600 | 0,026 s |
| `shared` sem proteção | π entre 0,237711600 e 0,419337600 | 0,049 s |
| `shared` + `critical` por ponto | π = 3,141793600 | 0,750 s |
| `private` local + `critical` por thread | π = 3,141793600 | 0,004 s |

A referência contou 7.854.484 pontos dentro do círculo em todas as repetições. A
versão com corrida contou entre 594.279 e 1.048.344 e variou a cada execução. As
duas versões protegidas reproduziram exatamente a referência.

O `critical` por ponto foi correto, mas muito mais lento por causa da
serialização. A agregação privada reduziu a sincronização e obteve cerca de 6,5
vezes o desempenho sequencial. Como esses tempos são curtos, o valor exato do
speedup pode variar com a carga da máquina.

O `lastprivate` devolveu o ponto `(0,208288741; -0,438420464)`, igual ao ponto
calculado separadamente para o índice 9.999.999. A semente externa permaneceu
2026, como esperado com `firstprivate`.

## 6. Conclusão

As amostras de Monte Carlo são independentes, mas o contador usado para reuni-las
não é. A paralelização direta criou uma corrida e produziu valores incorretos.
`critical` restaurou a correção, e o uso de um contador privado por thread evitou
que a trava fosse acessada a cada iteração. As cláusulas de escopo deixaram
explícito quais dados pertencem a cada thread e quais são compartilhados;
`default(none)` reforçou essa documentação com verificação do compilador.

<div class="quebra"></div>

## 7. Código-fonte

{{CODIGO:pi_monte_carlo.c}}
