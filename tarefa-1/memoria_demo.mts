/* ============================================================================
 * memoria_demo.mts - O que o Node "esconde" do modelo de memoria compartilhada
 *
 * Tres experimentos com 4 workers incrementando um contador 1 milhao de vezes
 * cada. O resultado correto e' sempre 4.000.000.
 *
 *   A) variavel normal      -> o worker recebe uma COPIA; o pai nao ve nada
 *   B) SharedArrayBuffer    -> memoria de verdade compartilhada, SEM protecao
 *                              => condicao de corrida, resultado errado
 *   C) SharedArrayBuffer    -> com Atomics (secao critica)
 *                              => resultado correto
 *
 * Em C com OpenMP, o caso (B) e' o que voce escreve SEM QUERER na primeira
 * tentativa. Em Node, voce precisa se esforcar para conseguir chegar nele.
 *
 * Executar: node --experimental-strip-types memoria_demo.mts
 * ==========================================================================*/

import { Worker, isMainThread, workerData, parentPort } from 'node:worker_threads';

const N_WORKERS = 4;
const INCREMENTOS = 1_000_000;
const ESPERADO = N_WORKERS * INCREMENTOS;

type Modo = 'copia' | 'corrida' | 'atomico';

interface DadosWorker {
  modo: Modo;
  objeto?: { valor: number };
  buffer?: SharedArrayBuffer;
}

/* ---------------------------------------------------------------------------
 * LADO DO WORKER
 * -------------------------------------------------------------------------*/
if (!isMainThread) {
  const dados = workerData as DadosWorker;

  if (dados.modo === 'copia') {
    // dados.objeto NAO e' o objeto do pai. E' uma copia feita pelo
    // structured clone na hora de criar o worker. Mexer aqui nao afeta la'.
    for (let i = 0; i < INCREMENTOS; i++) {
      dados.objeto!.valor++;
    }
    parentPort!.postMessage(dados.objeto!.valor);
  } else {
    // Aqui sim: o Int32Array aponta para os MESMOS bytes de memoria fisica
    // que o pai e os outros workers enxergam.
    const contador = new Int32Array(dados.buffer!);

    if (dados.modo === 'corrida') {
      // Incremento NAO atomico. Cada "++" sao tres passos separados:
      //   1. ler contador[0] da memoria para um registrador
      //   2. somar 1 no registrador
      //   3. escrever o registrador de volta na memoria
      // Se outro worker escrever entre os passos 1 e 3, aquele incremento
      // e' PERDIDO. Isso e' a condicao de corrida.
      for (let i = 0; i < INCREMENTOS; i++) {
        contador[0] = contador[0] + 1;
      }
    } else {
      // Atomics.add faz ler-somar-escrever como UMA operacao indivisivel,
      // usando a instrucao LOCK XADD do processador. Ninguem consegue se
      // meter no meio. E' o equivalente do #pragma omp atomic.
      for (let i = 0; i < INCREMENTOS; i++) {
        Atomics.add(contador, 0, 1);
      }
    }
    parentPort!.postMessage(contador[0]);
  }
}

/* ---------------------------------------------------------------------------
 * LADO DO PAI
 * -------------------------------------------------------------------------*/
else {
  const caminho = import.meta.filename;

  function rodaWorkers(dados: DadosWorker): Promise<{ ms: number }> {
    const inicio = performance.now();

    return new Promise((resolve, reject) => {
      let vivos = N_WORKERS;

      for (let i = 0; i < N_WORKERS; i++) {
        const w = new Worker(caminho, { workerData: dados });
        w.on('error', reject);
        w.on('exit', () => {
          if (--vivos === 0) resolve({ ms: performance.now() - inicio });
        });
      }
    });
  }

  console.log(`${N_WORKERS} workers x ${INCREMENTOS.toLocaleString('pt-BR')} incrementos`);
  console.log(`Resultado correto esperado: ${ESPERADO.toLocaleString('pt-BR')}\n`);

  /* --- A) Variavel normal: nao ha compartilhamento nenhum ---------------- */
  const objeto = { valor: 0 };
  await rodaWorkers({ modo: 'copia', objeto });
  console.log('A) Objeto JS normal passado para os workers');
  console.log(`   valor no processo pai depois de tudo : ${objeto.valor}`);
  console.log(`   (cada worker mexeu na propria copia; o pai nunca viu)\n`);

  /* --- B) SharedArrayBuffer sem protecao: condicao de corrida ------------ */
  const bufB = new SharedArrayBuffer(4); // 4 bytes = um int32
  const arrB = new Int32Array(bufB);
  const tB = await rodaWorkers({ modo: 'corrida', buffer: bufB });
  const perdidos = ESPERADO - arrB[0];
  console.log('B) SharedArrayBuffer com "contador[0] = contador[0] + 1"');
  console.log(`   resultado : ${arrB[0].toLocaleString('pt-BR')}`);
  console.log(`   perdidos  : ${perdidos.toLocaleString('pt-BR')} (${((perdidos / ESPERADO) * 100).toFixed(1)}%)`);
  console.log(`   tempo     : ${tB.ms.toFixed(0)} ms  <-- ERRADO e nao deterministico\n`);

  /* --- C) SharedArrayBuffer com Atomics: correto ------------------------- */
  const bufC = new SharedArrayBuffer(4);
  const arrC = new Int32Array(bufC);
  const tC = await rodaWorkers({ modo: 'atomico', buffer: bufC });
  console.log('C) SharedArrayBuffer com Atomics.add()');
  console.log(`   resultado : ${arrC[0].toLocaleString('pt-BR')}`);
  console.log(`   tempo     : ${tC.ms.toFixed(0)} ms  <-- CORRETO, e mais lento`);
  console.log(`   (a serializacao na secao critica custa ${(tC.ms / tB.ms).toFixed(1)}x)`);
}
