/* ============================================================================
 * pi_workers.mts - O equivalente do pi_omp.c em Node, com worker_threads
 *
 * Mesma serie, mesmo N, mesma varredura de 1 a 20 threads. A diferenca esta'
 * no que precisa ser escrito a mao: nao existe `reduction`, nao existe
 * `parallel for`. Voce particiona o intervalo, cria os workers, recebe as somas
 * parciais e combina - tudo manualmente.
 *
 * A coluna que interessa para o TCC e' OVERHEAD: quanto do tempo total NAO foi
 * calculo. E' o preco de criar isolates e trocar mensagens, que o OpenMP nao
 * paga porque as threads ja compartilham o mesmo espaco de memoria.
 *
 * Executar: node --experimental-strip-types pi_workers.mts
 *           node --experimental-strip-types pi_workers.mts 100000000
 * ==========================================================================*/

import { Worker, isMainThread, workerData, parentPort } from 'node:worker_threads';
import os from 'node:os';

const PI_REF = Math.PI;

interface Tarefa {
  lo: number;
  hi: number;
}

interface Parcial {
  soma: number;
  msCalculo: number;
}

/* ---------------------------------------------------------------------------
 * LADO DO WORKER
 * -------------------------------------------------------------------------*/
if (!isMainThread) {
  const { lo, hi } = workerData as Tarefa;

  /* Mesma reescrita do sinal do pi_omp.c: (k & 1) em vez de sinal = -sinal.
   * A dependencia carregada pelo laco impediria o particionamento tanto aqui
   * quanto la' - o problema e' do algoritmo, nao da linguagem. */
  function somaParcial(de: number, ate: number): number {
    let soma = 0.0;
    for (let k = de; k < ate; k++) {
      const sinal = k & 1 ? -1.0 : 1.0;
      soma += sinal / (2 * k + 1);
    }
    return soma;
  }

  /* Aquecimento OBRIGATORIO aqui - e este e' um custo que o OpenMP nao tem.
   * Cada worker e' um isolate novo, com JIT frio: o laco comeca interpretado e
   * so' depois de alguns milhares de iteracoes vira codigo de maquina. Sem
   * aquecer, workers com fatias pequenas nunca chegam a ser otimizados e o
   * speedup medido fica falsamente ruim. O tempo do aquecimento aparece no
   * OVERHEAD da tabela, que e' onde ele deve mesmo aparecer. */
  somaParcial(0, 200_000);

  const t0 = performance.now();
  const soma = somaParcial(lo, hi);
  const msCalculo = performance.now() - t0;

  parentPort!.postMessage({ soma, msCalculo } satisfies Parcial);
}

/* ---------------------------------------------------------------------------
 * LADO DO PAI
 * -------------------------------------------------------------------------*/
else {
  const caminho = import.meta.filename;
  const N = process.argv[2] ? Number(process.argv[2]) : 1_000_000_000;
  const MAX_WORKERS = os.availableParallelism();

  interface Medida {
    pi: number;
    msTotal: number;
    msCalculoMax: number;
  }

  /* Aqui esta' todo o trabalho que o `#pragma omp parallel for reduction(+:soma)`
   * faz numa linha: particionar, criar, coletar e combinar. */
  function rodaComWorkers(nWorkers: number): Promise<Medida> {
    const t0 = performance.now();

    return new Promise((resolve, reject) => {
      const parciais: Parcial[] = [];
      const fatia = Math.floor(N / nWorkers);

      for (let i = 0; i < nWorkers; i++) {
        const lo = i * fatia;
        const hi = i === nWorkers - 1 ? N : lo + fatia; // o ultimo pega o resto

        const w = new Worker(caminho, { workerData: { lo, hi } satisfies Tarefa });

        w.on('message', (p: Parcial) => {
          parciais.push(p);

          if (parciais.length === nWorkers) {
            // A "reducao" feita a mao: somar as parciais.
            const soma = parciais.reduce((acc, p) => acc + p.soma, 0);
            resolve({
              pi: 4.0 * soma,
              msTotal: performance.now() - t0,
              msCalculoMax: Math.max(...parciais.map((p) => p.msCalculo)),
            });
          }
        });
        w.on('error', reject);
      }
    });
  }

  console.log(`Node ${process.version} | availableParallelism: ${MAX_WORKERS} | N = ${N}\n`);
  console.log(`=== Speedup com worker_threads (N = ${N}) ===\n`);

  const cab =
    `  ${'WORKERS'.padStart(7)}  ${'APROXIMACAO'.padEnd(17)} ${'ERRO'.padStart(10)}  ` +
    `${'TOTAL(s)'.padStart(9)}  ${'CALCULO(s)'.padStart(10)}  ${'OVERHEAD(s)'.padStart(11)}  ` +
    `${'SPEEDUP'.padStart(8)}  ${'EFIC'.padStart(6)}  ${'KARP-FLATT'.padStart(10)}`;
  console.log(cab);
  console.log('  ' + '-'.repeat(cab.length));

  let base = 0;

  for (let w = 1; w <= MAX_WORKERS; w++) {
    const m = await rodaComWorkers(w);
    const s = m.msTotal / 1000;
    const calc = m.msCalculoMax / 1000;
    const over = s - calc;

    if (w === 1) base = s;
    const speedup = base / s;
    const efic = speedup / w;
    const karp = w > 1 ? (1 / speedup - 1 / w) / (1 - 1 / w) : NaN;

    console.log(
      `  ${String(w).padStart(7)}  ${m.pi.toFixed(15)}  ${Math.abs(PI_REF - m.pi).toExponential(3).padStart(10)}  ` +
        `${s.toFixed(4).padStart(9)}  ${calc.toFixed(4).padStart(10)}  ${over.toFixed(4).padStart(11)}  ` +
        `${speedup.toFixed(2).padStart(8)}  ${(efic * 100).toFixed(0).padStart(5)}%  ` +
        `${(w > 1 ? karp.toFixed(4) : '-').padStart(10)}`,
    );
  }

  console.log('\n  TOTAL    = tempo de parede de ponta a ponta (criar + calcular + coletar)');
  console.log('  CALCULO  = tempo do worker mais lento so' + ' no laco numerico');
  console.log('  OVERHEAD = TOTAL - CALCULO = criacao de isolates, JIT frio e mensagens');
  console.log('             E' + "' o custo que o OpenMP nao paga, porque threads");
  console.log('             compartilham memoria em vez de trocar mensagens.');
}
