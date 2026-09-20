#!/usr/bin/env python3
"""Adapta o gerador comum para o relatorio da Tarefa 12."""

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
        "tarefa": "Tarefa 12",
        "data": "20 de setembro de 2026",
    }
    globais["PASTA_RELATORIO"] = PASTA_RELATORIO
    globais["PASTA_TAREFA"] = PASTA_TAREFA
    globais["CSS"] = globais["CSS"] + """
img {
  display: block;
  width: 100%;
  margin: 2.5mm auto 0;
  page-break-inside: avoid;
}

.legenda {
  font-family: "Segoe UI", sans-serif;
  font-size: 8pt;
  color: #666;
  text-align: center;
  margin: 1mm 0 3.5mm;
  page-break-before: avoid;
}
"""
    return modulo["main"]()


if __name__ == "__main__":
    raise SystemExit(main())
