# Guia para apresentação — Tarefa 6

Este arquivo é um apoio para a explicação presencial e não faz parte do PDF.

## Fala curta

> Eu sorteei pontos uniformes no quadrado de lado 2. A fração que cai no círculo
> unitário aproxima π dividido por 4, então multiplico a fração por 4. Cada ponto
> pode ser processado independentemente, mas o contador não pode ser incrementado
> simultaneamente sem proteção. O `critical` corrige a corrida. Para não pagar uma
> trava em milhões de incrementos, cada thread acumula numa variável privada e
> entra no `critical` apenas uma vez. As demais cláusulas deixam claro o escopo e
> demonstram a entrada e a saída de dados da região paralela.

## Ordem para explicar o código

1. `gerar_ponto` gera as mesmas amostras para todas as versões usando semente e
   índice, sem estado global compartilhado.
2. `contar_sequencial` fornece a contagem de referência.
3. `contar_com_corrida` usa somente `parallel for`; o `dentro++` compartilhado
   perde incrementos.
4. `contar_critical_por_ponto` protege cada incremento e fica correto, mas lento.
5. `contar_critical_local` separa `parallel` e `for`, cria um contador por thread
   e faz apenas uma atualização protegida por thread.
6. `main` executa as quatro versões, compara as contagens e verifica o ponto
   devolvido por `lastprivate`.

## Perguntas e respostas

### Por que multiplicar a proporção por 4?

O círculo unitário tem área π e o quadrado `[-1,1] × [-1,1]` tem área 4. A
probabilidade de um ponto uniforme cair no círculo é π/4.

### O que faz `parallel`?

Cria uma equipe de threads e faz todas executarem o bloco estruturado. Sem uma
diretiva de divisão de trabalho, todas repetiriam o mesmo código do bloco.

### O que faz `for`?

Dentro de uma região paralela, divide as iterações entre as threads existentes.
Ele não cria sozinho uma nova equipe. `parallel for` é a forma combinada das duas
operações quando não é necessário código por thread ao redor do laço.

### Por que `dentro++` causa uma corrida?

Porque é uma sequência de leitura, soma e escrita. Threads podem ler o mesmo valor
e gravar o mesmo sucessor, perdendo incrementos. O resultado passa a depender da
intercalação não controlada.

### O que o `critical` faz?

Permite que somente uma thread por vez execute o bloco protegido. É semelhante a
adquirir e liberar uma trava. Ele garante a atualização do total, mas espera e
serialização podem custar caro quando o bloco é visitado muitas vezes.

### O que poderia ser usado sem `critical`?

`atomic` protegeria o incremento individual. Para uma soma, `reduction` seria a
solução OpenMP mais natural: ela cria acumuladores privados e combina os valores
no fim. A tarefa pediu explicitamente `critical`, então a implementação faz
manualmente uma agregação local.

### O que faz `private`?

Cria uma instância independente para cada thread e não copia o valor anterior.
Por isso o contador privado precisa receber zero dentro da região.

### O que faz `firstprivate`?

Também cria uma instância por thread, mas a inicializa copiando o valor que
existia antes da região. A semente 2026 é copiada para todas as threads.

### O que faz `lastprivate`?

Copia para a variável externa o valor produzido pela última iteração lógica do
laço. Não significa a thread que acabou por último. No teste, ele recupera o ponto
do índice 9.999.999.

### O que faz `shared`?

Mantém uma única instância acessível por todas as threads. Leituras simultâneas
são seguras se ninguém escreve; escritas concorrentes precisam ser analisadas e,
neste contador, protegidas.

### `private` e as outras cláusulas têm custo?

Podem usar armazenamento por thread e fazer cópias na entrada ou saída. Para
escalares esse custo costuma ser pequeno. O custo dominante aqui foi o `critical`
executado milhões de vezes, por causa da contenção e serialização.

### Como `default(none)` ajuda?

Ele obriga a classificar explicitamente as variáveis externas usadas na região.
O compilador acusa uma variável esquecida. Isso funciona como documentação
verificável, mas não substitui o raciocínio: declarar incorretamente um contador
como `shared` ainda pode criar corrida.

### Mais amostras pioram a estimativa?

Não numa implementação correta: o erro estatístico tende a cair como `1/√N`. Na
versão defeituosa, mais operações oferecem mais oportunidades para a corrida,
mas o erro causado por ela não segue uma regra monotônica.

### Por que não usar `rand()`?

Um gerador com estado global poderia introduzir contenção ou outra disputa. O
programa deriva cada ponto do índice, deixando as iterações independentes e
garantindo que todas as versões recebam as mesmas amostras.

### Resultado igual uma vez provaria que não há corrida?

Não. A corrida é identificada pelos acessos sem sincronização, e não somente pela
saída observada. Uma intercalação favorável pode esconder o problema numa execução.

## Se o professor apontar um trecho do código

### Linhas 11–14: bibliotecas

- `omp.h`: declara funções do runtime OpenMP, como `omp_get_wtime` (as diretivas
  `#pragma` são reconhecidas pelo compilador);
- `stdint.h`: fornece o tipo inteiro `uint64_t`;
- `stdio.h`: fornece `printf` e `fprintf`;
- `stdlib.h`: conversão dos argumentos e códigos de sucesso ou falha.

### Linhas 17–27: `misturar` e `uniforme`

`misturar` é uma função de mistura de bits baseada no SplitMix64. XOR,
deslocamentos e multiplicações fazem entradas próximas produzirem bits bem
diferentes. Os números hexadecimais são constantes conhecidas dessa mistura; não
foram escolhidos pelo OpenMP nem representam π. `UINT64_C` garante que elas sejam
constantes de 64 bits.

Depois, `uniforme` mantém 53 bits e divide por `2^53`, produzindo um `double` no
intervalo `[0,1)`. São números pseudoaleatórios reproduzíveis, não aleatoriedade
criptográfica. Esta parte é apenas o gerador das amostras; o assunto principal da
tarefa começa nos laços de contagem.

### Linhas 30–35: `gerar_ponto`

O índice `i` define duas entradas diferentes: `semente + 2i` para `x` e a entrada
seguinte para `y`. Multiplicar um número de `[0,1)` por 2 e subtrair 1 o leva para
`[-1,1)`. Os parâmetros `double *x` e `double *y` são ponteiros porque a função
precisa devolver duas coordenadas; `*x` e `*y` escrevem nos endereços recebidos.

O benefício para o teste paralelo é importante: o ponto depende apenas de
`semente` e `i`. Portanto, a ordem de execução e o número de threads não mudam as
amostras.

### Linhas 37–50: versão sequencial

Inicializa `dentro` com zero, gera cada ponto, testa `x²+y²≤1` e incrementa o
contador. Ela é a referência porque há somente um fluxo de execução e nenhuma
corrida.

### Linhas 53–67: versão com corrida

`parallel for` cria as threads e divide o laço. O índice `i` é automaticamente
privado pelo OpenMP. Como `x` e `y` são declarados dentro do corpo, cada execução
possui seus próprios temporários. `dentro`, declarado fora, é compartilhado e seu
incremento não está protegido: essa é a linha propositalmente incorreta.

### Linhas 70–89: `critical` por ponto

O `critical` envolve somente `dentro++`, então os incrementos não se sobrepõem.
A contagem fica correta. O problema é desempenho: aproximadamente 7,85 milhões
de acertos disputam a mesma região crítica.

O índice declarado no cabeçalho do `for` já possui escopo privado predeterminado;
por isso ele não precisa aparecer numa cláusula mesmo com `default(none)`. `x` e
`y` são locais ao corpo e também não precisam ser listados.

### Linhas 92–128: versão final

- `dentro` é o total compartilhado;
- `dentro_local`, `i`, `x` e `y` têm uma cópia por thread;
- `dentro_local = 0` é necessário porque `private` não copia nem inicializa o
  valor anterior;
- `omp for` reparte as iterações da equipe que já foi criada por `parallel`;
- cada thread incrementa somente seu contador local;
- ao sair do laço, cada thread soma uma vez seu subtotal no `critical`;
- a soma pode ocorrer em qualquer ordem, porque a adição dos inteiros produz o
  mesmo total.

Existe uma barreira implícita no fim de `omp for`: normalmente as threads esperam
todas concluírem o laço antes de seguir. Ela não é a proteção de `dentro`; quem
protege essa escrita continua sendo o `critical`.

### Por que `final_x` e `final_y` aparecem em `shared` e `lastprivate`?

No `parallel`, elas designam as variáveis externas compartilhadas que receberão o
resultado. No `omp for`, `lastprivate` cria cópias privadas durante o laço e, ao
final, copia para as variáveis externas o valor da iteração `amostras-1`. Sem isso,
escrever diretamente nas variáveis compartilhadas criaria outra corrida e não
garantiria o ponto da última iteração lógica.

Essas coordenadas foram incluídas apenas para demonstrar `lastprivate`; removê-las
não alteraria a estimativa de π.

### Por que `semente` é `firstprivate` em vez de `shared`?

Para demonstrar uma cópia inicial privada. Como o código somente lê a semente,
`shared(semente)` também seria correto e provavelmente teria resultado semelhante.
`private(semente)` sozinho não seria correto, pois a cópia começaria sem valor
definido e teria de ser inicializada dentro da região.

### Por que `ultimo_x` e `ultimo_y` não aparecem nas cláusulas?

Os ponteiros recebidos pela função só são usados depois que a região paralela
termina, nas linhas 125–126. Dentro da região são usadas `final_x` e `final_y`, que
estão declaradas em `shared` e `lastprivate`.

### Linhas 130–136: impressão

Calcula `4*dentro/amostras`. Os `casts` para `double` impedem divisão inteira.
Também imprime a diferença entre cada contagem e a referência sequencial.

### Linhas 138–201: `main`

Os operadores `? :` escolhem o argumento informado ou um valor padrão. O padrão é
10 milhões de amostras, o número de processadores disponíveis e semente 2026.
`omp_set_dynamic(0)` impede o runtime de alterar dinamicamente o tamanho da equipe;
`omp_set_num_threads` solicita a quantidade escolhida.

`omp_get_wtime()` retorna tempo de parede. O programa registra o instante antes e
depois de cada função e calcula a diferença. Por fim, executa as quatro versões,
imprime seus resultados e confirma que `lastprivate` devolveu o mesmo ponto que
uma geração direta do último índice.

## Perguntas mais perigosas

### Por que não basta tornar `dentro` privado?

Porque cada thread terminaria com uma parcela isolada e a variável externa
continuaria zero. É necessário combinar as parcelas, neste caso somando-as ao
total compartilhado dentro de `critical`.

### `critical` deixa o programa sequencial?

Somente o bloco protegido é executado por uma thread por vez. A geração e o teste
dos pontos continuam paralelos. Na versão por ponto, o bloco é acessado tantas
vezes que a serialização domina; na versão final, ele é acessado apenas uma vez
por thread.

### `default(none)` torna o programa correto?

Não. Ele obriga a declarar o escopo, mas o programador ainda pode escolher um
escopo inadequado ou esquecer a sincronização de uma escrita compartilhada.

### Por que o resultado com corrida às vezes fica muito abaixo de π?

Porque muitos incrementos são perdidos. O programa divide uma contagem incorreta
pelo número total original de amostras, então a proporção fica artificialmente
pequena.

### `lastprivate` escolhe a thread que terminou por último?

Não. Escolhe o valor correspondente à última iteração na ordem sequencial do
laço, aqui `i = amostras - 1`.

### Por que a versão incorreta pode ser mais lenta que a sequencial?

Várias threads escrevendo a mesma região de memória provocam contenção e tráfego
de coerência de cache, além do custo de criar e coordenar a equipe. Paralelizar não
garante ganho, principalmente quando existe compartilhamento inadequado.

### O programa calcula π exatamente?

Não. Ele calcula uma estimativa estatística. Com a semente fixa, a execução é
reproduzível, mas continua representando uma amostra pseudoaleatória finita.

## O que realmente precisa ser memorizado

1. A razão geométrica: `π ≈ 4*dentro/total`.
2. A corrida ocorre em `dentro++`, não na geração ou no teste dos pontos.
3. `critical` permite uma thread por vez no bloco.
4. Um contador privado por thread reduz milhões de travas para uma trava por
   thread.
5. `private` não copia o valor; `firstprivate` copia a entrada; `lastprivate`
   copia a última iteração para fora; `shared` mantém uma única instância.
6. `default(none)` obriga a explicitar o escopo, mas não substitui sincronização.
