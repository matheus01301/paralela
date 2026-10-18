#!/usr/bin/env python3
"""Gera o PDF da Tarefa 16 usando o gerador dos relatorios anteriores."""

import runpy
from pathlib import Path


PASTA_RELATORIO = Path(__file__).resolve().parent
GERADOR_BASE = PASTA_RELATORIO.parent.parent / "tarefa-2" / "relatorio" / "build_pdf.py"


def main() -> int:
    modulo = runpy.run_path(str(GERADOR_BASE), run_name="gerador_relatorio_base")
    globais = modulo["main"].__globals__
    globais["IDENTIFICACAO"] = {
        "disciplina": "Programação Paralela",
        "aluno": "Matheus Marinho",
        "tarefa": "Tarefa 16",
        "data": "29 de setembro de 2026",
    }
    globais["PASTA_RELATORIO"] = PASTA_RELATORIO
    globais["PASTA_TAREFA"] = PASTA_RELATORIO.parent
    globais["CSS"] += "pre { font-size: 6.6pt; line-height: 1.20; }"
    return modulo["main"]()


if __name__ == "__main__":
    raise SystemExit(main())
