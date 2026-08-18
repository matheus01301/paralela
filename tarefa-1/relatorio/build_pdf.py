#!/usr/bin/env python3
"""
Gera relatorio.pdf a partir de relatorio.md.

Cadeia: markdown-it-py (Markdown -> HTML) + Pygments (realce de sintaxe)
        + Chrome headless (HTML -> PDF).

Nao depende de rede, de pandoc nem de LaTeX.

Uso:  python build_pdf.py
"""

import re
import shutil
import subprocess
import sys
from pathlib import Path

from markdown_it import MarkdownIt
from pygments import highlight
from pygments.formatters import HtmlFormatter
from pygments.lexers import CLexer, get_lexer_by_name

# --------------------------------------------------------------------------
# IDENTIFICACAO -- preencher
# --------------------------------------------------------------------------
IDENT = {
    "instituicao": "",
    "disciplina": "Programação Paralela",
    "professor": "",
    "aluno": "Matheus Marinho",
    "tarefa": "Tarefa 1",
    "data": "18 de agosto de 2026",
}

AQUI = Path(__file__).resolve().parent
RAIZ = AQUI.parent  # tarefa-1/, onde ficam os fontes

# --------------------------------------------------------------------------
# Realce de sintaxe
# --------------------------------------------------------------------------
# `linenos="inline"` coloca o numero da linha dentro do <pre>, em vez de numa
# tabela de duas colunas. Numa tabela, uma quebra de pagina no meio do bloco
# desalinha a coluna de numeros do codigo; inline quebra corretamente.
FORMATTER = HtmlFormatter(
    style="friendly",
    linenos="inline",
    nowrap=False,
    cssclass="hl",
)


def realce_arquivo(nome: str) -> str:
    """Le um fonte do diretorio da tarefa e devolve o HTML com realce."""
    caminho = RAIZ / nome
    codigo = caminho.read_text(encoding="utf-8", errors="replace")
    linhas = codigo.count("\n")
    corpo = highlight(codigo, CLexer(), FORMATTER)
    return (
        f'<div class="fonte">'
        f'<div class="fonte-cabecalho">{nome} &nbsp;·&nbsp; {linhas} linhas</div>'
        f"{corpo}</div>"
    )


def realce_trecho(codigo: str, lang: str) -> str:
    """Realce dos blocos ``` do proprio Markdown (sem numeracao de linha)."""
    try:
        lexer = get_lexer_by_name(lang or "text")
    except Exception:
        lexer = get_lexer_by_name("text")
    fmt = HtmlFormatter(style="friendly", nowrap=False, cssclass="hl")
    return highlight(codigo, lexer, fmt)


# --------------------------------------------------------------------------
# CSS
# --------------------------------------------------------------------------
CSS = (
    """
@page { size: A4; margin: 15mm 16mm 15mm 16mm; }

* { box-sizing: border-box; }

body {
  font-family: "Georgia", "Cambria", serif;
  font-size: 9.3pt;
  line-height: 1.35;
  color: #1a1a1a;
  margin: 0;
  hyphens: auto;
}

/* ---- Cabecalho de identificacao ----
   Em vez de uma capa de pagina inteira: a identificacao ocupa 25mm no topo da
   primeira pagina e o conteudo comeca logo abaixo. Economiza uma pagina inteira
   num relatorio que precisa ser curto. */
.cabecalho {
  border-bottom: 2px solid #8a1c1c;
  padding-bottom: 3mm;
  margin-bottom: 5mm;
}
.cabecalho .linha {
  display: flex;
  justify-content: space-between;
  font-family: "Segoe UI", sans-serif;
  font-size: 8.5pt;
  color: #666;
}
.cabecalho .linha + .linha { margin-top: 1mm; }
.cabecalho .tarefa {
  font-weight: 700;
  color: #8a1c1c;
  letter-spacing: .1em;
  text-transform: uppercase;
}
.cabecalho h1 {
  font-size: 15pt;
  line-height: 1.3;
  margin: 3mm 0 2mm 0;
  border: none;
  padding: 0;
}

/* ---- Titulos ---- */
h1, h2, h3 { font-weight: 700; page-break-after: avoid; break-after: avoid; }
h2 {
  font-size: 12.5pt;
  margin: 4mm 0 1.5mm 0;
  padding-bottom: 1.2mm;
  border-bottom: 1.5px solid #d8d8d8;
}
h3 { font-size: 10.5pt; margin: 3.5mm 0 1.5mm 0; color: #333; }
h2 + h3 { margin-top: 3mm; }

p { margin: 0 0 1.8mm 0; text-align: justify; }

strong { color: #000; }
em { color: #333; }

ol, ul { margin: 0 0 2.5mm 0; padding-left: 5.5mm; }
li { margin-bottom: 1.2mm; text-align: justify; }

/* ---- Formulas em destaque ---- */
.formula {
  font-family: "Cambria Math", "Georgia", serif;
  font-size: 10.5pt;
  text-align: center;
  margin: 2.5mm 0 2.5mm 0;
  padding: 2mm;
  background: #f7f7f4;
  border-left: 3px solid #c9c9c0;
  page-break-inside: avoid;
}

/* ---- Tabelas ---- */
table {
  border-collapse: collapse;
  width: 100%;
  margin: 2mm 0 3mm 0;
  font-size: 8.4pt;
  font-family: "Segoe UI", "Helvetica Neue", sans-serif;
  /* `avoid` na tabela inteira empurra tabela grande para a pagina seguinte e
     deixa meia pagina branca. Deixamos a tabela quebrar, mas nunca no meio de
     uma linha -- e o cabecalho se repete no topo da continuacao. */
  page-break-inside: auto;
}
tr { page-break-inside: avoid; }
thead { display: table-header-group; }
th {
  background: #f0efec;
  border-bottom: 1.2px solid #999;
  border-top: 1.2px solid #999;
  padding: 1.2mm 1.8mm;
  text-align: left;
  font-weight: 700;
}
td { border-bottom: 0.5px solid #e2e2e2; padding: 1mm 1.8mm; vertical-align: top; }
tr:last-child td { border-bottom: 1.2px solid #999; }
td:empty { padding: 0; }

/* numeros em tabela: monoespacado tabular, para as colunas alinharem */
td { font-variant-numeric: tabular-nums; }

/* ---- Codigo ---- */
code {
  font-family: "Cascadia Mono", "Consolas", monospace;
  font-size: 9pt;
  background: #f2f2ef;
  padding: 0.3mm 1mm;
  border-radius: 2px;
}

pre {
  font-family: "Cascadia Mono", "Consolas", monospace;
  font-size: 7.2pt;
  line-height: 1.3;
  background: #fbfbf9;
  border: 0.6px solid #dcdcd4;
  border-left: 2.5px solid #8a1c1c;
  padding: 2.5mm 3mm;
  margin: 3mm 0;
  white-space: pre-wrap;
  word-wrap: break-word;
  overflow-wrap: break-word;
}
pre code { background: none; padding: 0; font-size: inherit; }

/* numeros de linha do Pygments (linenos="inline") */
pre .linenos, pre .lineno {
  color: #b4b4ac;
  user-select: none;
  padding-right: 2.5mm;
}

/* bloco do fonte completo */
.fonte { margin-top: 4mm; }
.fonte-cabecalho {
  font-family: "Segoe UI", sans-serif;
  font-size: 8.5pt;
  font-weight: 700;
  color: #fff;
  background: #4a4a44;
  padding: 1.4mm 3mm;
  letter-spacing: .03em;
}
.fonte pre { margin-top: 0; border-top: none; }

/* uma pagina nova para o codigo-fonte final */
.quebra { page-break-before: always; }

/* citacao em bloco */
blockquote {
  margin: 3mm 0;
  padding: 3mm 4mm;
  background: #f7f5f0;
  border-left: 3px solid #8a1c1c;
  font-size: 10.5pt;
  page-break-inside: avoid;
}
blockquote p { margin: 0; text-align: left; }
"""
    + FORMATTER.get_style_defs(".hl")
)



# --------------------------------------------------------------------------
# Montagem
# --------------------------------------------------------------------------
def cabecalho_html(titulo_h1: str) -> str:
    i = IDENT
    curso = " &nbsp;·&nbsp; ".join(
        valor for valor in (i["instituicao"], i["disciplina"]) if valor
    )
    autoria = " &nbsp;·&nbsp; ".join(
        valor for valor in (i["aluno"], i["professor"]) if valor
    )
    return f"""
<div class="cabecalho">
  <div class="linha"><span>{curso}</span>
                     <span class="tarefa">{i["tarefa"]}</span></div>
  <h1>{titulo_h1}</h1>
  <div class="linha"><span>{autoria}</span>
                     <span>{i["data"]}</span></div>
</div>
"""


def imprime(navegador: str, html: Path, pdf: Path) -> None:
    """HTML -> PDF pelo Chrome headless, sem o cabecalho/rodape padrao dele."""
    subprocess.run(
        [
            navegador,
            "--headless",
            "--disable-gpu",
            "--no-pdf-header-footer",
            f"--print-to-pdf={pdf}",
            html.as_uri(),
        ],
        check=True,
        capture_output=True,
    )


def numera_paginas(navegador: str, pdf: Path, primeira_sem_numero: bool = True) -> None:
    """Sobrepoe o numero da pagina no rodape.

    O Chrome pela linha de comando so oferece o rodape padrao dele, que carrega a
    URL do arquivo. Em vez disso: descobrimos quantas paginas o PDF tem, geramos
    um segundo PDF que contem apenas os numeros nas mesmas posicoes, e mesclamos
    os dois. A capa (pagina 1) fica sem numero, como e' convencional.
    """
    try:
        from pypdf import PdfReader, PdfWriter
    except ImportError:
        print("aviso: pypdf ausente, PDF gerado sem numeracao de paginas")
        return

    leitor = PdfReader(str(pdf))
    total = len(leitor.pages)

    paginas = "".join(
        f'<div class="pg">{"" if (i == 1 and primeira_sem_numero) else i}</div>'
        for i in range(1, total + 1)
    )
    overlay_html = f"""<!doctype html><html><head><meta charset="utf-8"><style>
@page {{ size: A4; margin: 0; }}
body {{ margin: 0; font-family: "Segoe UI", sans-serif; }}
/* box-sizing: border-box e' obrigatorio aqui: com content-box o padding soma
   a' altura, a div passa de 297mm e cada uma vaza para uma segunda pagina --
   o overlay sai com o dobro de paginas e desalinhado. 296mm em vez de 297mm
   por causa do arredondamento de mm para pixel na impressao. */
.pg {{
  box-sizing: border-box;
  height: 296mm; page-break-after: always;
  display: flex; align-items: flex-end; justify-content: center;
  padding-bottom: 11mm; font-size: 8.5pt; color: #8c8c8c;
}}
</style></head><body>{paginas}</body></html>"""

    tmp_html = AQUI / "_numeros.html"
    tmp_pdf = AQUI / "_numeros.pdf"
    tmp_html.write_text(overlay_html, encoding="utf-8")
    imprime(navegador, tmp_html, tmp_pdf)

    numeros = PdfReader(str(tmp_pdf))
    escritor = PdfWriter()
    for i, pagina in enumerate(leitor.pages):
        if i < len(numeros.pages):
            pagina.merge_page(numeros.pages[i])
        escritor.add_page(pagina)

    saida = AQUI / "_saida.pdf"
    with open(saida, "wb") as f:
        escritor.write(f)
    saida.replace(pdf)

    tmp_html.unlink(missing_ok=True)
    tmp_pdf.unlink(missing_ok=True)
    primeira = 2 if primeira_sem_numero else 1
    print(f"        {total} paginas, numeradas de {primeira} a {total}")


def markdown() -> MarkdownIt:
    md = MarkdownIt("commonmark", {"html": True, "typographer": False})
    md.enable("table")

    # Realce dos blocos ``` embutidos no Markdown.
    def render_fence(self, tokens, idx, options, env):
        tok = tokens[idx]
        return realce_trecho(tok.content, (tok.info or "").strip())

    md.add_render_rule("fence", render_fence)
    return md


def monta(md_nome: str, css: str, titulo: str, com_capa: bool) -> tuple[Path, int]:
    """Markdown -> HTML. Devolve o caminho do HTML e quantos fontes foram embutidos."""
    md_path = AQUI / md_nome
    texto = md_path.read_text(encoding="utf-8")

    cabecalho = ""
    if com_capa:
        # O h1 do Markdown vira o titulo dentro do cabecalho de identificacao,
        # em vez de ficar repetido no corpo.
        m = re.match(r"^# (.*?)\n", texto)
        titulo_h1 = m.group(1) if m else titulo
        texto = texto[m.end() :] if m else texto
        cabecalho = cabecalho_html(titulo_h1)

    corpo = markdown().render(texto)

    # Substitui os marcadores {{CODIGO:arquivo}} pelo fonte realçado. A quebra de
    # pagina fica no proprio Markdown, antes do titulo da secao: se fosse inserida
    # aqui, o titulo ficaria sozinho no pe da pagina anterior.
    def sub_codigo(m):
        return realce_arquivo(m.group(1))

    corpo, n_fontes = re.subn(r"<p>\{\{CODIGO:([^}]+)\}\}</p>", sub_codigo, corpo)

    html = f"""<!doctype html>
<html lang="pt-BR"><head><meta charset="utf-8">
<title>{titulo}</title>
<style>{css}</style></head>
<body>{cabecalho}{corpo}</body></html>
"""
    html_path = AQUI / (md_path.stem + ".html")
    html_path.write_text(html, encoding="utf-8")
    return html_path, n_fontes


def acha_navegador() -> str | None:
    candidatos = [
        shutil.which("chrome"),
        shutil.which("msedge"),
        r"C:\Program Files\Google\Chrome\Application\chrome.exe",
        r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe",
        r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
        "/usr/bin/google-chrome",
        "/usr/bin/chromium",
    ]
    return next((c for c in candidatos if c and Path(c).exists()), None)


# O documento gerado. `capa` liga o cabecalho de identificacao do IDENT.
DOCUMENTOS = [
    {
        "md": "relatorio.md",
        "titulo": f"{IDENT['tarefa']} — Aproximação de π por séries",
        "capa": True,
    },
]


def main() -> int:
    navegador = acha_navegador()
    if navegador is None:
        print("erro: Chrome/Edge nao encontrado", file=sys.stderr)
        return 2

    for doc in DOCUMENTOS:
        if not (AQUI / doc["md"]).exists():
            print(f"aviso: {doc['md']} nao encontrado, pulando", file=sys.stderr)
            continue

        html_path, n_fontes = monta(doc["md"], CSS, doc["titulo"], doc["capa"])
        pdf_path = html_path.with_suffix(".pdf")

        imprime(navegador, html_path, pdf_path)
        numera_paginas(navegador, pdf_path, primeira_sem_numero=False)

        fontes = f", {n_fontes} fonte(s) embutido(s)" if n_fontes else ""
        print(f"PDF   : {pdf_path.name}  ({pdf_path.stat().st_size/1024:.0f} KB{fontes})")

    if any("PREENCHER" in str(v) for v in IDENT.values()):
        print("\nATENCAO: o cabecalho tem campos PREENCHER no dicionario IDENT.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
