#!/usr/bin/env python3
"""Gera os SVG do ping-pong a partir de resultados.txt."""

import math
import statistics
from pathlib import Path

PASTA = Path(__file__).resolve().parent
DADOS = PASTA.parent / "resultados.txt"

SERIES = ["mesmo_no", "nos_diferentes"]
COR = {"mesmo_no": "#2a78d6", "nos_diferentes": "#eb6834"}
ROTULO = {"mesmo_no": "mesmo nó (memória)", "nos_diferentes": "nós diferentes (rede)"}

SUPERFICIE = "#fcfcfb"
TINTA = "#0b0b0b"
TINTA2 = "#52514e"
GRADE = "#e3e3de"
EIXO = "#a8a8a2"

L, R, T, B = 70, 26, 52, 48
LARG, ALT = 760, 360


def ler():
    """Mediana das execuções de cada cenário: (bytes, tempo de ida em µs, GB/s)."""
    tempos = {s: {} for s in SERIES}
    atual = None
    for linha in DADOS.read_text(encoding="utf-8").splitlines():
        if linha.startswith("## "):
            atual = linha[3:].strip()
            continue
        if atual not in tempos or not linha[:1].isdigit():
            continue
        b, total, ida, banda = linha.split(",")
        tempos[atual].setdefault(int(b), []).append(float(ida))
    dados = {}
    for s in SERIES:
        dados[s] = []
        for b in sorted(tempos[s]):
            ida = statistics.median(tempos[s][b])
            dados[s].append((b, ida, b / ida / 1000.0))
    return dados


def rotulo_bytes(b):
    if b >= 1024 * 1024:
        return f"{b // (1024 * 1024)} MB"
    if b >= 1024:
        return f"{b // 1024} KB"
    return f"{b} B"


def rotulo_valor(v):
    return f"{v:g}"


def desenhar(dados, indice, rotulo_y, arquivo):
    todos = [p for s in SERIES for p in dados[s]]
    xmin = math.log2(min(p[0] for p in todos))
    xmax = math.log2(max(p[0] for p in todos))
    ymin = math.floor(math.log10(min(p[indice] for p in todos)))
    ymax = math.ceil(math.log10(max(p[indice] for p in todos)))

    def px(b):
        return L + (math.log2(b) - xmin) / (xmax - xmin) * (LARG - L - R)

    def py(v):
        return ALT - B - (math.log10(v) - ymin) / (ymax - ymin) * (ALT - B - T)

    p = [f'<rect x="0" y="0" width="{LARG}" height="{ALT}" fill="{SUPERFICIE}"/>']

    x = L
    for s in SERIES:
        p.append(
            f'<line x1="{x}" y1="22" x2="{x + 16}" y2="22" stroke="{COR[s]}" '
            f'stroke-width="2.5" stroke-linecap="round"/>'
            f'<circle cx="{x + 8}" cy="22" r="3.6" fill="{COR[s]}" '
            f'stroke="{SUPERFICIE}" stroke-width="1.2"/>'
            f'<text x="{x + 22}" y="25.5" font-size="11.5" fill="{TINTA}">{ROTULO[s]}</text>'
        )
        x += 40 + len(ROTULO[s]) * 6.4

    for e in range(ymin, ymax + 1):
        v = 10.0 ** e
        y = py(v)
        p.append(f'<line x1="{L}" y1="{y:.1f}" x2="{LARG - R}" y2="{y:.1f}" '
                 f'stroke="{GRADE}" stroke-width="1"/>')
        p.append(f'<text x="{L - 10}" y="{y + 4:.1f}" font-size="11.5" fill="{TINTA2}" '
                 f'text-anchor="end">{rotulo_valor(v)}</text>')

    for e in range(int(xmin), int(xmax) + 1, 3):
        xx = px(2 ** e)
        p.append(f'<text x="{xx:.1f}" y="{ALT - B + 20}" font-size="11.5" fill="{TINTA2}" '
                 f'text-anchor="middle">{rotulo_bytes(2 ** e)}</text>')

    p.append(f'<line x1="{L}" y1="{ALT - B}" x2="{LARG - R}" y2="{ALT - B}" '
             f'stroke="{EIXO}" stroke-width="1.2"/>')
    p.append(f'<text x="{(L + LARG - R) / 2:.0f}" y="{ALT - 8}" font-size="12" '
             f'fill="{TINTA2}" text-anchor="middle">tamanho da mensagem</text>')
    p.append(f'<text transform="translate(16,{(T + ALT - B) / 2:.0f}) rotate(-90)" '
             f'font-size="12" fill="{TINTA2}" text-anchor="middle">{rotulo_y}</text>')

    for s in SERIES:
        serie = dados[s]
        caminho = " ".join(
            f'{"M" if i == 0 else "L"}{px(b):.1f},{py(pt[indice]):.1f}'
            for i, pt in enumerate(serie) for b in [pt[0]]
        )
        p.append(f'<path d="{caminho}" fill="none" stroke="{COR[s]}" stroke-width="2" '
                 f'stroke-linejoin="round" stroke-linecap="round"/>')
        for pt in serie:
            p.append(f'<circle cx="{px(pt[0]):.1f}" cy="{py(pt[indice]):.1f}" r="3.6" '
                     f'fill="{COR[s]}" stroke="{SUPERFICIE}" stroke-width="1.4"/>')

    svg = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {LARG} {ALT}" '
           f'width="100%" font-family="Segoe UI, Helvetica, Arial, sans-serif">'
           + "".join(p) + "</svg>")
    (PASTA / arquivo).write_text(svg, encoding="utf-8")


def main():
    dados = ler()
    desenhar(dados, 1, "tempo de ida (µs)", "tempo.svg")
    desenhar(dados, 2, "largura de banda (GB/s)", "banda.svg")
    print("gerados: tempo.svg, banda.svg")


if __name__ == "__main__":
    raise SystemExit(main())
