/* ============================================================================
 * overhead_test.mts - Do que exatamente e' feito o overhead de um worker?
 *
 * O pi_workers.mts mediu ~65 ms fixos + ~3,4 ms por worker, mas nao disse de
 * QUE isso e' feito. Aqui a conta e' decomposta em duas familias de custo, que
 * sao independentes uma da outra:
 *
 *   PARTE 1 - custo de EXISTIR: criar o isolate, subir o Node dentro dele,
 *             compilar o modulo, aquecer o JIT. Pago por worker criado,
 *             independe do tamanho do dado.
 *
 *   PARTE 2 - custo de MOVER DADO: copia estruturada, transferencia e memoria
 *             compartilhada. Independe da criacao; depende do tamanho.
 *
 * Executar: node --experimental-strip-types overhead_test.mts
 * ==========================================================================*/

import { Worker } from 'node:worker_threads';

const REPS = 7;

const mediana = (v: number[]): number => [...v].sort((a, b) => a - b)[Math.floor(v.length / 2)];
const ms = (v: number) => v.toFixed(2).padStart(8);

/* Cria um worker a partir de codigo em string, espera ele morrer, devolve o
 * tempo de parede total. */
function criaEspera(codigo: string): Promise<number> {
  const t0 = performance.now();
  return new Promise((res, rej) => {
    const w = new Worker(codigo, { eval: true });
    w.on('error', rej);
    w.on('exit', () => res(performance.now() - t0));
  });
}

/* =========================== PARTE 1 ===================================== */
console.log('=== PARTE 1: custo de criar um worker (nao depende do dado) ===\n');

/* A) Worker completamente vazio. Isola o custo de: criar o isolate V8, alocar
 *    heap e GC proprios, e subir o bootstrap do Node dentro dele. */
const vazio: number[] = [];
for (let i = 0; i < REPS; i++) vazio.push(await criaEspera(''));

/* B) Worker que so' importa um modulo interno do Node. A diferenca para (A) e'
 *    o custo de resolver e compilar modulo dentro do isolate novo. */
const comImport: number[] = [];
for (let i = 0; i < REPS; i++) {
  comImport.push(await criaEspera('const os = require("node:os"); os.cpus();'));
}

/* C) Worker que roda 200.000 iteracoes do laco - exatamente o aquecimento que o
 *    pi_workers.mts faz. A diferenca para (B) e' o custo do JIT aquecer DE NOVO
 *    neste isolate. Em C esse custo simplesmente nao existe: o codigo ja esta'
 *    compilado no binario e e' compartilhado por todas as threads. */
const comJit: number[] = [];
for (let i = 0; i < REPS; i++) {
  comJit.push(
    await criaEspera(`
      let s = 0;
      for (let k = 0; k < 200000; k++) s += (k & 1 ? -1 : 1) / (2 * k + 1);
      if (s === 42) console.log(s);
    `),
  );
}

const mVazio = mediana(vazio);
const mImport = mediana(comImport);
const mJit = mediana(comJit);

console.log(`  A) isolate + bootstrap do Node   ${ms(mVazio)} ms`);
console.log(`  B) A + resolver/compilar modulo  ${ms(mImport)} ms   (+${(mImport - mVazio).toFixed(2)} ms)`);
console.log(`  C) B + aquecer o JIT do laco     ${ms(mJit)} ms   (+${(mJit - mImport).toFixed(2)} ms)`);
console.log(`\n  Nenhuma dessas etapas moveu um unico byte de dado do usuario.`);

/* =========================== PARTE 2 ===================================== */
console.log('\n=== PARTE 2: custo de mover dado (worker ja criado e quente) ===\n');

/* Um unico worker persistente, criado uma vez. Ele so' responde "ok" ao receber
 * qualquer mensagem - assim o tempo medido e' de ida e volta da MENSAGEM, sem
 * nenhum custo de criacao misturado. */
const eco = new Worker(
  `const { parentPort } = require('node:worker_threads');
   parentPort.on('message', () => parentPort.postMessage('ok'));`,
  { eval: true },
);

function rodada(fabrica: () => { payload: unknown; transfer?: Transferable[] }): Promise<number> {
  const { payload, transfer } = fabrica();
  const t0 = performance.now();
  return new Promise((res) => {
    eco.once('message', () => res(performance.now() - t0));
    eco.postMessage(payload, (transfer ?? []) as never);
  });
}

console.log(`  ${'TAMANHO'.padStart(9)}  ${'COPIA'.padStart(9)}  ${'TRANSFER'.padStart(9)}  ${'SHARED'.padStart(9)}`);
console.log('  ' + '-'.repeat(44));

for (const mb of [1, 10, 100, 500]) {
  const bytes = mb * 1024 * 1024;

  // Copia estruturada: o ArrayBuffer e' duplicado byte a byte no outro isolate.
  const copia: number[] = [];
  for (let i = 0; i < REPS; i++) copia.push(await rodada(() => ({ payload: new ArrayBuffer(bytes) })));

  // Transferivel: o buffer MUDA DE DONO. Nada e' copiado - so' o ponteiro passa
  // e o buffer original fica "detached", inutilizavel do lado de ca'.
  const transf: number[] = [];
  for (let i = 0; i < REPS; i++) {
    transf.push(
      await rodada(() => {
        const buf = new ArrayBuffer(bytes);
        return { payload: buf, transfer: [buf] };
      }),
    );
  }

  // SharedArrayBuffer: os dois isolates apontam para os MESMOS bytes. Ninguem
  // copia, ninguem perde o acesso - e por isso e' o unico que permite corrida.
  const shared: number[] = [];
  for (let i = 0; i < REPS; i++) {
    shared.push(await rodada(() => ({ payload: new SharedArrayBuffer(bytes) })));
  }

  console.log(
    `  ${(mb + ' MB').padStart(9)}  ${ms(mediana(copia))}  ${ms(mediana(transf))}  ${ms(mediana(shared))}`,
  );
}

await eco.terminate();

console.log('\n  COPIA    = structured clone: duplica os bytes no outro isolate');
console.log('  TRANSFER = muda o dono do buffer; o lado de ca fica sem acesso');
console.log('  SHARED   = os dois lados enxergam os mesmos bytes, ao mesmo tempo');
