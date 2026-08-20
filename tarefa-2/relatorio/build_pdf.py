#!/usr/bin/env python3
"""Gera relatorio.pdf a partir de relatorio.md."""

import re
import shutil
import subprocess
import sys
from pathlib import Path

from markdown_it import MarkdownIt
from pygments import highlight
from pygments.formatters import HtmlFormatter
from pygments.lexers import CLexer, get_lexer_by_name


IDENTIFICACAO = {
    "disciplina": "Programação Paralela",
    "aluno": "Matheus Marinho",
    "tarefa": "Tarefa 2",
    "data": "20 de agosto de 2026",
}

PASTA_RELATORIO = Path(__file__).resolve().parent
PASTA_TAREFA = PASTA_RELATORIO.parent

FORMATADOR = HtmlFormatter(
    style="friendly",
    linenos="inline",
    nowrap=False,
    cssclass="codigo",
)


CSS = (
    """
@page { size: A4; margin: 14mm 16mm; }

* { box-sizing: border-box; }

body {
  font-family: "Georgia", "Cambria", serif;
  font-size: 9.2pt;
  line-height: 1.34;
  color: #1a1a1a;
  margin: 0;
  hyphens: auto;
}

.cabecalho {
  border-bottom: 2px solid #8a1c1c;
  padding-bottom: 3mm;
  margin-bottom: 4mm;
}

.cabecalho .linha {
  display: flex;
  justify-content: space-between;
  font-family: "Segoe UI", sans-serif;
  font-size: 8.5pt;
  color: #666;
}

.cabecalho .tarefa {
  color: #8a1c1c;
  font-weight: 700;
  letter-spacing: .1em;
  text-transform: uppercase;
}

.cabecalho h1 {
  font-size: 15pt;
  line-height: 1.25;
  margin: 3mm 0 2mm;
  border: 0;
}

h1, h2, h3 {
  page-break-after: avoid;
  break-after: avoid;
}

h2 {
  font-size: 12pt;
  margin: 4mm 0 1.5mm;
  padding-bottom: 1mm;
  border-bottom: 1.5px solid #d8d8d8;
}

p {
  margin: 0 0 1.8mm;
  text-align: justify;
}

code {
  font-family: "Cascadia Mono", "Consolas", monospace;
  font-size: 8.7pt;
  background: #f2f2ef;
  padding: .2mm .8mm;
  border-radius: 2px;
}

.formula {
  font-family: "Cambria Math", "Georgia", serif;
  font-size: 10.5pt;
  text-align: center;
  margin: 2.5mm 0;
  padding: 2mm;
  background: #f7f7f4;
  border-left: 3px solid #c9c9c0;
  page-break-inside: avoid;
}

table {
  border-collapse: collapse;
  width: 100%;
  margin: 2mm 0 3mm;
  font-family: "Segoe UI", sans-serif;
  font-size: 8.2pt;
  page-break-inside: auto;
}

thead { display: table-header-group; }
tr { page-break-inside: avoid; }

th {
  background: #f0efec;
  border-top: 1.2px solid #999;
  border-bottom: 1.2px solid #999;
  padding: 1.1mm 1.6mm;
  text-align: left;
}

td {
  border-bottom: .5px solid #e2e2e2;
  padding: .9mm 1.6mm;
  vertical-align: top;
  font-variant-numeric: tabular-nums;
}

tr:last-child td { border-bottom: 1.2px solid #999; }

pre {
  font-family: "Cascadia Mono", "Consolas", monospace;
  font-size: 6.8pt;
  line-height: 1.25;
  background: #fbfbf9;
  border: .6px solid #dcdcd4;
  border-left: 2.5px solid #8a1c1c;
  padding: 2.2mm 2.8mm;
  margin: 2.5mm 0;
  white-space: pre-wrap;
  overflow-wrap: break-word;
}

pre code { background: none; padding: 0; font-size: inherit; }
pre .linenos, pre .lineno { color: #b4b4ac; padding-right: 2mm; }

.fonte-cabecalho {
  font-family: "Segoe UI", sans-serif;
  font-size: 8.3pt;
  font-weight: 700;
  color: #fff;
  background: #4a4a44;
  padding: 1.3mm 2.8mm;
}

.fonte pre { margin-top: 0; border-top: 0; }
.quebra { page-break-before: always; }
"""
    + FORMATADOR.get_style_defs(".codigo")
)


def realcar_fonte(nome: str) -> str:
    caminho = PASTA_TAREFA / nome
    codigo = caminho.read_text(encoding="utf-8")
    corpo = highlight(codigo, CLexer(), FORMATADOR)
    linhas = len(codigo.splitlines())
    return (
        '<div class="fonte">'
        f'<div class="fonte-cabecalho">{nome} &nbsp;·&nbsp; {linhas} linhas</div>'
        f"{corpo}</div>"
    )


def criar_markdown() -> MarkdownIt:
    md = MarkdownIt("commonmark", {"html": True})
    md.enable("table")

    def renderizar_bloco(self, tokens, indice, options, env):
        token = tokens[indice]
        try:
            lexer = get_lexer_by_name((token.info or "text").strip())
        except Exception:
            lexer = get_lexer_by_name("text")
        formatador = HtmlFormatter(style="friendly", nowrap=False, cssclass="codigo")
        return highlight(token.content, lexer, formatador)

    md.add_render_rule("fence", renderizar_bloco)
    return md


def encontrar_navegador() -> str | None:
    candidatos = [
        shutil.which("chrome"),
        shutil.which("msedge"),
        r"C:\Program Files\Google\Chrome\Application\chrome.exe",
        r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
    ]
    return next((item for item in candidatos if item and Path(item).exists()), None)


def main() -> int:
    navegador = encontrar_navegador()
    if navegador is None:
        print("Erro: Chrome ou Edge não encontrado.", file=sys.stderr)
        return 1

    caminho_md = PASTA_RELATORIO / "relatorio.md"
    texto = caminho_md.read_text(encoding="utf-8")

    titulo = re.match(r"^# (.*?)\n", texto)
    titulo_texto = titulo.group(1) if titulo else "Tarefa 2"
    if titulo:
        texto = texto[titulo.end():]

    corpo = criar_markdown().render(texto)
    corpo = re.sub(
        r"<p>\{\{CODIGO:([^}]+)\}\}</p>",
        lambda resultado: realcar_fonte(resultado.group(1)),
        corpo,
    )

    ident = IDENTIFICACAO
    cabecalho = f"""
<div class="cabecalho">
  <div class="linha"><span>{ident['disciplina']}</span>
                     <span class="tarefa">{ident['tarefa']}</span></div>
  <h1>{titulo_texto}</h1>
  <div class="linha"><span>{ident['aluno']}</span><span>{ident['data']}</span></div>
</div>
"""

    html = f"""<!doctype html>
<html lang="pt-BR"><head><meta charset="utf-8">
<title>{titulo_texto}</title><style>{CSS}</style></head>
<body>{cabecalho}{corpo}</body></html>
"""

    caminho_html = PASTA_RELATORIO / "relatorio.html"
    caminho_pdf = PASTA_RELATORIO / "relatorio.pdf"
    caminho_html.write_text(html, encoding="utf-8")

    subprocess.run(
        [
            navegador,
            "--headless",
            "--disable-gpu",
            "--no-pdf-header-footer",
            f"--print-to-pdf={caminho_pdf}",
            caminho_html.as_uri(),
        ],
        check=True,
        capture_output=True,
    )

    print(f"PDF: {caminho_pdf} ({caminho_pdf.stat().st_size / 1024:.0f} KB)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
