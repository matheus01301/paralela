/* ============================================================================
 * pi_serie.ts - Aproximacao de PI por series matematicas (porte do pi_serie.c)
 *
 * Mesma estrutura do programa em C, de proposito: mesmas series, mesmos N,
 * mesma medicao de tempo. So assim a comparacao entre as duas linguagens e'
 * honesta - se a estrutura mudasse, estariamos medindo a mudanca, nao a
 * linguagem.
 *
 * Executar:  node --experimental-strip-types pi_serie.ts
 *            node --experimental-strip-types pi_serie.ts 7   (vai ate 10^7)
 *
 * O Node 22 executa TypeScript direto apagando os tipos (type stripping).
 * Nao ha compilacao, nao ha tsc, nao ha bundler - os tipos sao apagados e o
 * JavaScript resultante e' entregue ao V8.
 * ==========================================================================*/

/* Valor de referencia de PI. Diferente do C, aqui Math.PI E' padrao da
 * linguagem - e vale exatamente o mesmo double que a constante do C. */
const PI_REF: number = Math.PI;

const MAX_EXPOENTE = 9;

/* ---------------------------------------------------------------------------
 * MEDICAO DE TEMPO
 *
 * Mesma distincao do C, com as APIs do Node:
 *   performance.now()    -> tempo de parede, em milissegundos, monotonico
 *   process.cpuUsage()   -> tempo de CPU (user + system), em microssegundos
 *
 * performance.now() e' o equivalente direto do QueryPerformanceCounter /
 * clock_gettime(CLOCK_MONOTONIC). NAO usamos Date.now(): ele e' o relogio de
 * parede do sistema, tem resolucao de ~1ms e pode ANDAR PARA TRAS quando o NTP
 * ajusta o horario da maquina - o que produziria tempos negativos.
 * -------------------------------------------------------------------------*/

function tempoParede(): number {
  return performance.now() / 1000; // ms -> s
}

function tempoCpu(): number {
  const u = process.cpuUsage();
  return (u.user + u.system) / 1_000_000; // microssegundos -> s
}

/* ---------------------------------------------------------------------------
 * AS SERIES
 *
 * Ponto crucial para a comparacao: o `number` do JavaScript E' um double IEEE
 * 754 de 64 bits, exatamente o mesmo tipo do `double` do C. Portanto os valores
 * numericos aqui saem IDENTICOS aos do C, bit a bit. O que muda entre as duas
 * linguagens e' so o TEMPO, nunca a acuracia.
 *
 * Note que os contadores sao `number`, nao `bigint`. Seria tentador usar bigint
 * para "inteiros de verdade", mas bigint aloca objetos no heap e e' ~50x mais
 * lento. Um number representa inteiros exatos ate 2^53 (~9e15), muito acima dos
 * 10^9 que precisamos.
 * -------------------------------------------------------------------------*/

function serieLeibniz(n: number): number {
  let soma = 0.0;
  let sinal = 1.0;

  for (let k = 0; k < n; k++) {
    soma += sinal / (2 * k + 1);
    sinal = -sinal;
  }
  return 4.0 * soma;
}

function serieNilakantha(n: number): number {
  let soma = 3.0;
  let sinal = 1.0;

  for (let k = 1; k <= n; k++) {
    const a = 2.0 * k;
    soma += (sinal * 4.0) / (a * (a + 1.0) * (a + 2.0));
    sinal = -sinal;
  }
  return soma;
}

/* ---------------------------------------------------------------------------
 * APOIO
 * -------------------------------------------------------------------------*/

function casasCorretas(erro: number): number {
  if (erro <= 0) return 17;
  return -Math.log10(erro);
}

/* Alinhamento de colunas. Em C o printf faz isso com "%12lld"; em JS/TS nao
 * existe printf, entao usamos padStart/padEnd na mao. */
const dir = (s: string | number, w: number) => String(s).padStart(w);
const esq = (s: string | number, w: number) => String(s).padEnd(w);

interface Resultado {
  erro: number;
  tempo: number;
}

function executa(
  nome: string,
  serie: (n: number) => number,
  n: number,
): Resultado {
  const t0Parede = tempoParede();
  const t0Cpu = tempoCpu();

  const aprox = serie(n); // <-- o trabalho de verdade

  const t1Cpu = tempoCpu();
  const t1Parede = tempoParede();

  const erro = Math.abs(PI_REF - aprox);
  const tempo = t1Parede - t0Parede;

  console.log(
    `  ${esq(nome, 11)} ${dir(n, 12)}  ${aprox.toFixed(15)}  ` +
      `${dir(erro.toExponential(3), 10)}  ${dir(casasCorretas(erro).toFixed(2), 6)}  ` +
      `${dir(tempo.toFixed(4), 9)}  ${dir((t1Cpu - t0Cpu).toFixed(4), 9)}`,
  );

  return { erro, tempo };
}

/* ---------------------------------------------------------------------------
 * AQUECIMENTO (warm-up) - nao existe equivalente em C
 *
 * O V8 e' um compilador JIT: na primeira vez, a funcao roda interpretada
 * (Ignition). Depois de algumas milhares de iteracoes ele detecta que o codigo
 * e' "quente", infere os tipos observados e recompila para codigo de maquina
 * otimizado (Maglev/TurboFan).
 *
 * Sem este aquecimento, as primeiras linhas da tabela mediriam o interpretador
 * e as ultimas o codigo otimizado - e a tabela mostraria uma aceleracao que e'
 * artefato do JIT, nao propriedade do algoritmo. Em C isso nao existe: o codigo
 * ja nasce compilado e a primeira iteracao roda tao rapido quanto a bilionesima.
 * -------------------------------------------------------------------------*/
function aquece(): void {
  for (let i = 0; i < 5; i++) {
    serieLeibniz(200_000);
    serieNilakantha(200_000);
  }
}

/* ---------------------------------------------------------------------------
 * PROGRAMA PRINCIPAL
 * -------------------------------------------------------------------------*/
function main(): void {
  // process.argv[0] = node, [1] = script, [2] = primeiro argumento do usuario
  const arg = process.argv[2];
  let expoenteMax = MAX_EXPOENTE;

  if (arg !== undefined) {
    expoenteMax = Number.parseInt(arg, 10);
    if (!Number.isInteger(expoenteMax) || expoenteMax < 1 || expoenteMax > MAX_EXPOENTE) {
      console.error(`Uso: node --experimental-strip-types pi_serie.ts [1..${MAX_EXPOENTE}]`);
      process.exit(1);
    }
  }

  console.log(`Runtime                   : Node ${process.version} (V8 ${process.versions.v8})`);
  console.log(`Valor de referencia de PI : ${PI_REF.toFixed(15)}`);
  console.log(`Precisao do tipo number   : double IEEE-754, ~15-17 digitos\n`);

  aquece();

  console.log(
    `  ${esq('SERIE', 11)} ${dir('N', 12)}  ${esq('APROXIMACAO', 17)} ` +
      `${dir('ERRO ABS', 10)}  ${dir('CASAS', 6)}  ${dir('T.PAREDE', 9)}  ${dir('T.CPU', 9)}`,
  );
  console.log('  ' + '-'.repeat(92));

  const ns: number[] = [];
  const leib: Resultado[] = [];
  const nila: Resultado[] = [];

  for (let e = 1; e <= expoenteMax; e++) {
    const n = 10 ** e;

    ns.push(n);
    leib.push(executa('Leibniz', serieLeibniz, n));
    nila.push(executa('Nilakantha', serieNilakantha, n));
    console.log('');
  }

  console.log('Fator de reducao do erro e de aumento do tempo a cada 10x em N:\n');
  console.log(
    `  ${dir('N', 14)}  ${dir('ERRO/10x LEIB', 13)}  ${dir('ERRO/10x NILA', 13)}  ${dir('TEMPO/10x', 12)}`,
  );
  console.log('  ' + '-'.repeat(60));

  for (let i = 1; i < ns.length; i++) {
    const fLeib = leib[i - 1].erro / leib[i].erro;
    const fNila = nila[i - 1].erro / nila[i].erro;
    const fTempo = leib[i - 1].tempo > 0 ? leib[i].tempo / leib[i - 1].tempo : 0;

    console.log(
      `  ${dir(ns[i], 14)}  ${dir(fLeib.toFixed(1), 13)}  ${dir(fNila.toFixed(1), 13)}  ${dir(fTempo.toFixed(1), 12)}`,
    );
  }

  console.log('\nLeitura: com Leibniz, 10x mais trabalho compra ~1 casa decimal.');
  console.log('Com Nilakantha, o mesmo trabalho compra ~3 casas - ate a serie');
  console.log('bater no limite de precisao do double (~1e-16), onde mais');
  console.log('iteracoes so acumulam erro de arredondamento e nao melhoram nada.');
}

main();
