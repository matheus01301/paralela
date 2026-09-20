#!/usr/bin/env python3
"""Gera os SVG de afinidade a partir de resultados.txt."""

import csv
import math
from pathlib import Path

PASTA = Path(__file__).resolve().parent
DADOS = PASTA.parent / "resultados.txt"

SUPERFICIE = "#fcfcfb"
TINTA = "#0b0b0b"
TINTA2 = "#52514e"
GRADE = "#e3e3de"
EIXO = "#a8a8a2"

L, R, T, B = 66, 26, 52, 48
LARG, ALT = 760, 360


def ler():
    campos = ["rotulo", "threads", "n", "tempo", "speedup",
              "eficiencia", "l3", "numa", "cpus"]
    dados = []
    for linha in DADOS.read_text(encoding="utf-8").splitlines():
        if linha.startswith("#") or linha.startswith("rotulo,") or "," not in linha:
            continue
        if linha.startswith("libgomp"):
            continue
        d = dict(zip(campos, next(csv.reader([linha]))))
        for k in ("threads", "n", "l3", "numa", "cpus"):
            d[k] = int(d[k])
        for k in ("tempo", "speedup", "eficiencia"):
            d[k] = float(d[k])
        dados.append(d)
    return dados


def escala_x(threads, dominio):
    return L + (math.log2(threads) / dominio) * (LARG - L - R)


def escala_y(valor, minimo, maximo):
    return ALT - B - ((valor - minimo) / (maximo - minimo)) * (ALT - B - T)


def legenda(series, cores, rotulos):
    partes = []
    x = L
    for s in series:
        partes.append(
            f'<line x1="{x}" y1="22" x2="{x + 16}" y2="22" stroke="{cores[s]}" '
            f'stroke-width="2.5" stroke-linecap="round"/>'
            f'<circle cx="{x + 8}" cy="22" r="3.6" fill="{cores[s]}" '
            f'stroke="{SUPERFICIE}" stroke-width="1.2"/>'
            f'<text x="{x + 22}" y="25.5" font-size="11.5" fill="{TINTA}">{rotulos[s]}</text>'
        )
        x += 24 + len(rotulos[s]) * 6.4
    return "".join(partes)


def grafico(dados, series, cores, rotulos, campo, rotulo_y,
            marcas_y, ymin, ymax, arquivo, log_y=False, ideal=False):
    pontos = [d for d in dados if d["rotulo"] in series]
    threads = sorted({d["threads"] for d in pontos})
    dominio_x = math.log2(max(threads))

    def posy(v):
        return escala_y(math.log2(max(v, 1e-9)) if log_y else v, ymin, ymax)

    p = [f'<rect x="0" y="0" width="{LARG}" height="{ALT}" fill="{SUPERFICIE}"/>',
         legenda(series, cores, rotulos)]

    for valor, texto in marcas_y:
        y = escala_y(valor, ymin, ymax)
        p.append(f'<line x1="{L}" y1="{y:.1f}" x2="{LARG - R}" y2="{y:.1f}" '
                 f'stroke="{GRADE}" stroke-width="1"/>')
        p.append(f'<text x="{L - 10}" y="{y + 4:.1f}" font-size="11.5" '
                 f'fill="{TINTA2}" text-anchor="end">{texto}</text>')

    for t in threads:
        p.append(f'<text x="{escala_x(t, dominio_x):.1f}" y="{ALT - B + 20}" '
                 f'font-size="11.5" fill="{TINTA2}" text-anchor="middle">{t}</text>')

    p.append(f'<line x1="{L}" y1="{ALT - B}" x2="{LARG - R}" y2="{ALT - B}" '
             f'stroke="{EIXO}" stroke-width="1.2"/>')
    p.append(f'<text x="{(L + LARG - R) / 2:.0f}" y="{ALT - 8}" font-size="12" '
             f'fill="{TINTA2}" text-anchor="middle">threads</text>')
    p.append(f'<text transform="translate(16,{(T + ALT - B) / 2:.0f}) rotate(-90)" '
             f'font-size="12" fill="{TINTA2}" text-anchor="middle">{rotulo_y}</text>')

    if ideal:
        x1, y1 = escala_x(threads[0], dominio_x), posy(threads[0])
        x2, y2 = escala_x(threads[-1], dominio_x), posy(threads[-1])
        p.append(f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" '
                 f'stroke="#9a9a95" stroke-width="1.6" stroke-dasharray="6 4"/>')
        p.append(f'<text x="{x2 - 6:.1f}" y="{y2 - 9:.1f}" font-size="11.5" '
                 f'fill="{TINTA2}" text-anchor="end">ideal</text>')

    for s in series:
        serie = sorted([d for d in pontos if d["rotulo"] == s],
                       key=lambda d: d["threads"])
        if not serie:
            continue
        caminho = " ".join(
            f'{"M" if i == 0 else "L"}{escala_x(d["threads"], dominio_x):.1f},'
            f'{posy(d[campo]):.1f}' for i, d in enumerate(serie))
        p.append(f'<path d="{caminho}" fill="none" stroke="{cores[s]}" '
                 f'stroke-width="2" stroke-linejoin="round" stroke-linecap="round"/>')
        for d in serie:
            p.append(f'<circle cx="{escala_x(d["threads"], dominio_x):.1f}" '
                     f'cy="{posy(d[campo]):.1f}" r="4" fill="{cores[s]}" '
                     f'stroke="{SUPERFICIE}" stroke-width="1.5"/>')

    svg = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {LARG} {ALT}" '
           f'width="100%" font-family="Segoe UI, Helvetica, Arial, sans-serif">'
           + "".join(p) + "</svg>")
    (PASTA / arquivo).write_text(svg, encoding="utf-8")


def main():
    dados = ler()

    principais = ["close_cores", "spread_cores", "close_threads",
                  "spread_threads", "sem_afinidade"]
    cores = {
        "close_cores": "#2a78d6",
        "spread_cores": "#eb6834",
        "close_threads": "#1baf7a",
        "spread_threads": "#eda100",
        "sem_afinidade": "#e87ba4",
        "so_interleave": "#2a78d6",
        "so_gomp_lista": "#eb6834",
        "so_gomp_espalha": "#1baf7a",
        "master_cores": "#eda100",
    }
    rotulos = {
        "close_cores": "close/cores",
        "spread_cores": "spread/cores",
        "close_threads": "close/threads",
        "spread_threads": "spread/threads",
        "sem_afinidade": "sem afinidade",
        "so_interleave": "numactl interleave",
        "so_gomp_lista": "GOMP lista 0-127",
        "so_gomp_espalha": "GOMP passo 16",
        "master_cores": "master/cores",
    }

    marcas_ef = [(v, f"{v:.1f}") for v in (0.0, 0.2, 0.4, 0.6, 0.8, 1.0, 1.2)]

    grafico(dados, principais, cores, rotulos, "eficiencia", "eficiencia",
            marcas_ef, 0.0, 1.30, "afinidade_eficiencia.svg")
    so = ["so_interleave", "so_gomp_lista", "so_gomp_espalha", "master_cores"]
    grafico(dados, so, cores, rotulos, "eficiencia", "eficiencia",
            marcas_ef, 0.0, 1.30, "so_eficiencia.svg")

    l3 = ["close_cores", "spread_cores", "close_threads", "spread_threads"]
    marcas_l3 = [(v, str(v)) for v in (0, 4, 8, 12, 16)]
    grafico(dados, l3, cores, rotulos, "l3", "dominios de L3 ocupados",
            marcas_l3, 0.0, 17.0, "afinidade_l3.svg")

    print("gerados: afinidade_eficiencia, afinidade_l3, so_eficiencia")


if __name__ == "__main__":
    raise SystemExit(main())
