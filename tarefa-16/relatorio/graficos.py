#!/usr/bin/env python3
"""Gera os SVG de tempo por fase a partir de resultados.txt (mediana de 5)."""

import statistics
from pathlib import Path

PASTA = Path(__file__).resolve().parent
DADOS = PASTA.parent / "resultados.txt"

FASES = [("distribuir", "#eb6834", "distribuir (Scatter + Bcast)"),
         ("calcular", "#2a78d6", "calcular"),
         ("juntar", "#1baf7a", "juntar (Gather)")]

SUPERFICIE = "#fcfcfb"
TINTA = "#0b0b0b"
TINTA2 = "#52514e"
GRADE = "#e3e3de"
EIXO = "#a8a8a2"

L, R, T, B = 66, 26, 52, 48
LARG, ALT = 760, 330


def ler():
    tempos = {}
    for linha in DADOS.read_text(encoding="utf-8").splitlines():
        if not linha[:1].isdigit():
            continue
        m, n, p, total, dist, calc, junt, y0, y1 = linha.split(",")
        if int(m) < 100:
            continue
        chave = (int(m), int(p))
        tempos.setdefault(chave, []).append((float(dist), float(calc), float(junt)))
    return {k: [statistics.median(t[i] for t in v) * 1000 for i in range(3)]
            for k, v in tempos.items()}


def desenhar(dados, m, passo_y, arquivo):
    procs = sorted(p for (mm, p) in dados if mm == m)
    maximo = max(sum(dados[(m, p)]) for p in procs)
    topo = passo_y * (int(maximo / passo_y) + 1)

    def py(v):
        return ALT - B - v / topo * (ALT - B - T)

    largura_util = LARG - L - R
    passo_x = largura_util / len(procs)
    barra = passo_x * 0.55

    p = [f'<rect x="0" y="0" width="{LARG}" height="{ALT}" fill="{SUPERFICIE}"/>']
    x = L
    for _, cor, rotulo in FASES:
        p.append(f'<rect x="{x}" y="15" width="13" height="13" rx="2" fill="{cor}"/>'
                 f'<text x="{x + 19}" y="26" font-size="11.5" fill="{TINTA}">{rotulo}</text>')
        x += 40 + len(rotulo) * 6.4

    v = 0.0
    while v <= topo + 1e-9:
        y = py(v)
        p.append(f'<line x1="{L}" y1="{y:.1f}" x2="{LARG - R}" y2="{y:.1f}" '
                 f'stroke="{GRADE}" stroke-width="1"/>')
        p.append(f'<text x="{L - 10}" y="{y + 4:.1f}" font-size="11.5" fill="{TINTA2}" '
                 f'text-anchor="end">{v:g}</text>')
        v += passo_y

    for k, proc in enumerate(procs):
        cx = L + passo_x * (k + 0.5)
        base = 0.0
        for (nome, cor, _), valor in zip(FASES, dados[(m, proc)]):
            y_topo, y_base = py(base + valor), py(base)
            p.append(f'<rect x="{cx - barra / 2:.1f}" y="{y_topo:.1f}" width="{barra:.1f}" '
                     f'height="{max(y_base - y_topo, 0):.1f}" fill="{cor}"/>')
            base += valor
        p.append(f'<text x="{cx:.1f}" y="{ALT - B + 20}" font-size="11.5" fill="{TINTA2}" '
                 f'text-anchor="middle">{proc}</text>')

    p.append(f'<line x1="{L}" y1="{ALT - B}" x2="{LARG - R}" y2="{ALT - B}" '
             f'stroke="{EIXO}" stroke-width="1.2"/>')
    p.append(f'<text x="{(L + LARG - R) / 2:.0f}" y="{ALT - 8}" font-size="12" '
             f'fill="{TINTA2}" text-anchor="middle">processos</text>')
    p.append(f'<text transform="translate(16,{(T + ALT - B) / 2:.0f}) rotate(-90)" '
             f'font-size="12" fill="{TINTA2}" text-anchor="middle">tempo (ms)</text>')

    svg = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {LARG} {ALT}" '
           f'width="100%" font-family="Segoe UI, Helvetica, Arial, sans-serif">'
           + "".join(p) + "</svg>")
    (PASTA / arquivo).write_text(svg, encoding="utf-8")


def main():
    dados = ler()
    desenhar(dados, 2048, 5, "fases_2048.svg")
    desenhar(dados, 8192, 20, "fases_8192.svg")
    print("gerados: fases_2048.svg, fases_8192.svg")


if __name__ == "__main__":
    raise SystemExit(main())
