#!/usr/bin/env python3
"""Adapta o gerador comum para o relatório da Tarefa 10."""

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
        "tarefa": "Tarefa 10",
        "data": "13 de setembro de 2026",
    }
    globais["PASTA_RELATORIO"] = PASTA_RELATORIO
    globais["PASTA_TAREFA"] = PASTA_TAREFA
    return modulo["main"]()


if __name__ == "__main__":
    raise SystemExit(main())
