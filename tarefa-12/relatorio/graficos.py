#!/usr/bin/env python3
"""Gera os SVG de escalabilidade a partir de resultados.txt."""

import csv
import math
from pathlib import Path

PASTA = Path(__file__).resolve().parent
DADOS = PASTA.parent / "resultados.txt"

SERIES = ["v0", "v1", "v2", "v3", "v4"]
COR = {
    "v0": "#2a78d6",
    "v1": "#eb6834",
    "v2": "#1baf7a",
    "v3": "#eda100",
    "v4": "#e87ba4",
}
ROTULO = {
    "v0": "v0 base",
    "v1": "v1 first touch",
    "v2": "v2 bordas em serie",
    "v3": "v3 bordas distribuidas",
    "v4": "v4 regiao unica",
}

SUPERFICIE = "#fcfcfb"
TINTA = "#0b0b0b"
TINTA2 = "#52514e"
GRADE = "#e3e3de"
EIXO = "#a8a8a2"
REFERENCIA = "#9a9a95"

L, R, T, B = 66, 26, 52, 48
LARG, ALT = 760, 360


def ler():
    linhas = []
    for linha in DADOS.read_text(encoding="utf-8").splitlines():
        if linha.startswith("#") or not linha.strip():
            continue
        if linha.startswith("modo,"):
            continue
        linhas.append(linha)
    campos = ["modo", "versao", "threads", "n", "tempo", "speedup", "eficiencia"]
    dados = []
    for row in csv.reader(linhas):
        d = dict(zip(campos, row))
        d["threads"] = int(d["threads"])
        d["n"] = int(d["n"])
        for k in ("tempo", "speedup", "eficiencia"):
            d[k] = float(d[k])
        dados.append(d)
    return dados


def escala_x(threads, dominio):
    return L + (math.log2(threads) / dominio) * (LARG - L - R)


def escala_y(valor, minimo, maximo):
    frac = (valor - minimo) / (maximo - minimo)
    return ALT - B - frac * (ALT - B - T)


def legenda():
    partes = []
    x = L
    for s in SERIES:
        partes.append(
            f'<line x1="{x}" y1="22" x2="{x + 16}" y2="22" stroke="{COR[s]}" '
            f'stroke-width="2.5" stroke-linecap="round"/>'
            f'<circle cx="{x + 8}" cy="22" r="3.6" fill="{COR[s]}" '
            f'stroke="{SUPERFICIE}" stroke-width="1.2"/>'
            f'<text x="{x + 22}" y="25.5" font-size="11.5" fill="{TINTA}">{ROTULO[s]}</text>'
        )
        x += 24 + len(ROTULO[s]) * 6.4
    return "".join(partes)


def moldura(marcas_x, dominio_x, marcas_y, ymin, ymax, rotulo_y):
    p = [
        f'<rect x="0" y="0" width="{LARG}" height="{ALT}" fill="{SUPERFICIE}"/>',
        legenda(),
    ]
    for valor, texto in marcas_y:
        y = escala_y(valor, ymin, ymax)
        p.append(
            f'<line x1="{L}" y1="{y:.1f}" x2="{LARG - R}" y2="{y:.1f}" '
            f'stroke="{GRADE}" stroke-width="1"/>'
        )
        p.append(
            f'<text x="{L - 10}" y="{y + 4:.1f}" font-size="11.5" fill="{TINTA2}" '
            f'text-anchor="end">{texto}</text>'
        )
    for t in marcas_x:
        x = escala_x(t, dominio_x)
        p.append(
            f'<text x="{x:.1f}" y="{ALT - B + 20}" font-size="11.5" fill="{TINTA2}" '
            f'text-anchor="middle">{t}</text>'
        )
    p.append(
        f'<line x1="{L}" y1="{ALT - B}" x2="{LARG - R}" y2="{ALT - B}" '
        f'stroke="{EIXO}" stroke-width="1.2"/>'
    )
    p.append(
        f'<text x="{(L + LARG - R) / 2:.0f}" y="{ALT - 8}" font-size="12" '
        f'fill="{TINTA2}" text-anchor="middle">threads</text>'
    )
    p.append(
        f'<text transform="translate(16,{(T + ALT - B) / 2:.0f}) rotate(-90)" '
        f'font-size="12" fill="{TINTA2}" text-anchor="middle">{rotulo_y}</text>'
    )
    return p


def desenhar(dados, modo, campo, rotulo_y, marcas_y, ymin, ymax, ideal, arquivo):
    pontos = [d for d in dados if d["modo"] == modo]
    threads = sorted({d["threads"] for d in pontos})
    dominio_x = math.log2(max(threads))

    if campo == "speedup":
        def posy(v):
            return escala_y(math.log2(max(v, 1e-9)), ymin, ymax)
    else:
        def posy(v):
            return escala_y(v, ymin, ymax)

    p = moldura(threads, dominio_x, marcas_y, ymin, ymax, rotulo_y)

    if ideal:
        x1, y1 = escala_x(threads[0], dominio_x), posy(threads[0])
        x2, y2 = escala_x(threads[-1], dominio_x), posy(threads[-1])
        p.append(
            f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" '
            f'stroke="{REFERENCIA}" stroke-width="1.6" stroke-dasharray="6 4"/>'
        )
        p.append(
            f'<text x="{x2 - 6:.1f}" y="{y2 - 9:.1f}" font-size="11.5" '
            f'fill="{TINTA2}" text-anchor="end">ideal</text>'
        )

    for s in SERIES:
        serie = sorted([d for d in pontos if d["versao"] == s], key=lambda d: d["threads"])
        if not serie:
            continue
        caminho = " ".join(
            f'{"M" if i == 0 else "L"}{escala_x(d["threads"], dominio_x):.1f},{posy(d[campo]):.1f}'
            for i, d in enumerate(serie)
        )
        p.append(
            f'<path d="{caminho}" fill="none" stroke="{COR[s]}" stroke-width="2" '
            f'stroke-linejoin="round" stroke-linecap="round"/>'
        )
        for d in serie:
            p.append(
                f'<circle cx="{escala_x(d["threads"], dominio_x):.1f}" '
                f'cy="{posy(d[campo]):.1f}" r="4" fill="{COR[s]}" '
                f'stroke="{SUPERFICIE}" stroke-width="1.5"/>'
            )

    svg = (
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {LARG} {ALT}" '
        f'width="100%" font-family="Segoe UI, Helvetica, Arial, sans-serif">'
        + "".join(p)
        + "</svg>"
    )
    (PASTA / arquivo).write_text(svg, encoding="utf-8")
    return arquivo


def main():
    dados = ler()

    desenhar(
        dados, "forte", "speedup", "speedup",
        [(math.log2(v), str(v)) for v in (1, 2, 4, 8, 16, 32, 64, 128)],
        0.0, 7.0, True, "forte_speedup.svg",
    )
    desenhar(
        dados, "forte", "eficiencia", "eficiencia",
        [(v, f"{v:.1f}") for v in (0.0, 0.2, 0.4, 0.6, 0.8, 1.0)],
        0.0, 1.05, False, "forte_eficiencia.svg",
    )
    desenhar(
        dados, "fraca", "eficiencia", "eficiencia",
        [(v, f"{v:.1f}") for v in (0.0, 0.2, 0.4, 0.6, 0.8, 1.0)],
        0.0, 1.05, False, "fraca_eficiencia.svg",
    )
    print("gerados: forte_speedup.svg, forte_eficiencia.svg, fraca_eficiencia.svg")


if __name__ == "__main__":
    raise SystemExit(main())
