#!/usr/bin/env python3
"""Adapta o gerador das tarefas anteriores para a Tarefa 5."""

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
        "tarefa": "Tarefa 5",
        "data": "28 de agosto de 2026",
    }
    globais["PASTA_RELATORIO"] = PASTA_RELATORIO
    globais["PASTA_TAREFA"] = PASTA_TAREFA
    return modulo["main"]()


if __name__ == "__main__":
    raise SystemExit(main())
