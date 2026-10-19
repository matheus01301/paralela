#!/usr/bin/env python3
"""Adapta o gerador comum para o relatório da Tarefa 11."""

import runpy
from pathlib import Path


PASTA_RELATORIO = Path(__file__).resolve().parent
PASTA_TAREFA = PASTA_RELATORIO.parent
GERADOR_BASE = PASTA_TAREFA.parent / "tarefa-2" / "relatorio" / "build_pdf.py"


def main() -> int:
    modulo = runpy.run_path(str(GERADOR_BASE), run_name="gerador_relatorio_base")
    globais = modulo["main"].__globals__
    globais["IDENTIFICACAO"] = {
        "disciplina": "Programação Paralela",
        "aluno": "Matheus Marinho",
        "tarefa": "Tarefa 11",
        "data": "18 de outubro de 2026",
    }
    globais["PASTA_RELATORIO"] = PASTA_RELATORIO
    globais["PASTA_TAREFA"] = PASTA_TAREFA
    globais["CSS"] += """
blockquote {
  margin: 2mm 0 3.5mm;
  padding: 1.8mm 3.5mm;
  border-left: 3px solid #b0342c;
  background: #faf5ef;
  font-style: italic;
  page-break-inside: avoid;
}

blockquote p {
  margin: 0;
}
"""
    return modulo["main"]()


if __name__ == "__main__":
    raise SystemExit(main())
