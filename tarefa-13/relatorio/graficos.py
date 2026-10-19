#!/usr/bin/env python3
"""Gera o SVG de tempo contra threads a partir de resultados.txt."""

import math
from pathlib import Path

PASTA = Path(__file__).resolve().parent
DADOS = PASTA.parent / "resultados.txt"

SERIES = ["false", "true", "close", "spread", "numactl_interleave"]
COR = {"false": "#9a9a95", "true": "#1baf7a", "close": "#2a78d6",
       "spread": "#eb6834", "numactl_interleave": "#8e5bd0"}
ROTULO = {"false": "false", "true": "true", "close": "close", "spread": "spread",
          "numactl_interleave": "close + numactl --interleave"}

SUPERFICIE = "#fcfcfb"
TINTA = "#0b0b0b"
TINTA2 = "#52514e"
GRADE = "#e3e3de"
EIXO = "#a8a8a2"

L, R, T, B = 66, 26, 52, 48
LARG, ALT = 760, 380


def ler():
    dados = {}
    for linha in DADOS.read_text(encoding="utf-8").splitlines():
        partes = linha.split(",")
        if len(partes) != 3 or not partes[1].isdigit():
            continue
        dados.setdefault(partes[0], []).append((int(partes[1]), float(partes[2])))
    return dados


def main():
    dados = ler()
    xmax = 7.0                       # log2(128)
    ymin, ymax = -2.0, 1.0           # 0,01 s a 10 s

    def px(t):
        return L + math.log2(t) / xmax * (LARG - L - R)

    def py(s):
        return ALT - B - (math.log10(s) - ymin) / (ymax - ymin) * (ALT - B - T)

    p = [f'<rect x="0" y="0" width="{LARG}" height="{ALT}" fill="{SUPERFICIE}"/>']
    x = L
    for s in SERIES:
        p.append(f'<line x1="{x}" y1="22" x2="{x + 16}" y2="22" stroke="{COR[s]}" '
                 f'stroke-width="2.5" stroke-linecap="round"/>'
                 f'<circle cx="{x + 8}" cy="22" r="3.6" fill="{COR[s]}" '
                 f'stroke="{SUPERFICIE}" stroke-width="1.2"/>'
                 f'<text x="{x + 22}" y="25.5" font-size="11.5" fill="{TINTA}">{ROTULO[s]}</text>')
        x += 40 + len(ROTULO[s]) * 6.4

    for e in range(int(ymin), int(ymax) + 1):
        y = py(10.0 ** e)
        p.append(f'<line x1="{L}" y1="{y:.1f}" x2="{LARG - R}" y2="{y:.1f}" '
                 f'stroke="{GRADE}" stroke-width="1"/>')
        p.append(f'<text x="{L - 10}" y="{y + 4:.1f}" font-size="11.5" fill="{TINTA2}" '
                 f'text-anchor="end">{10.0 ** e:g}</text>')
    for t in (1, 2, 4, 8, 16, 32, 64, 128):
        p.append(f'<text x="{px(t):.1f}" y="{ALT - B + 20}" font-size="11.5" fill="{TINTA2}" '
                 f'text-anchor="middle">{t}</text>')
    p.append(f'<line x1="{L}" y1="{ALT - B}" x2="{LARG - R}" y2="{ALT - B}" '
             f'stroke="{EIXO}" stroke-width="1.2"/>')
    p.append(f'<text x="{(L + LARG - R) / 2:.0f}" y="{ALT - 8}" font-size="12" '
             f'fill="{TINTA2}" text-anchor="middle">threads</text>')
    p.append(f'<text transform="translate(16,{(T + ALT - B) / 2:.0f}) rotate(-90)" '
             f'font-size="12" fill="{TINTA2}" text-anchor="middle">tempo (s)</text>')

    for s in SERIES:
        serie = sorted(dados[s])
        caminho = " ".join(f'{"M" if i == 0 else "L"}{px(t):.1f},{py(v):.1f}'
                           for i, (t, v) in enumerate(serie))
        p.append(f'<path d="{caminho}" fill="none" stroke="{COR[s]}" stroke-width="2" '
                 f'stroke-linejoin="round" stroke-linecap="round"/>')
        for t, v in serie:
            p.append(f'<circle cx="{px(t):.1f}" cy="{py(v):.1f}" r="3.6" fill="{COR[s]}" '
                     f'stroke="{SUPERFICIE}" stroke-width="1.4"/>')

    svg = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {LARG} {ALT}" '
           f'width="100%" font-family="Segoe UI, Helvetica, Arial, sans-serif">'
           + "".join(p) + "</svg>")
    (PASTA / "tempo.svg").write_text(svg, encoding="utf-8")
    print("gerado: tempo.svg")


if __name__ == "__main__":
    raise SystemExit(main())
