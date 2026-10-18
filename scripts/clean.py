#!/usr/bin/env python3
"""Remove generated build files while preserving sdkconfig and source files."""

import argparse
import shutil

from _common import DEFAULT_BUILD_DIR, PROJECT_ROOT, add_build_argument, entrypoint, read_build_description


def main():
    parser = argparse.ArgumentParser(description="Remove o diretório de build e preserva sdkconfig.")
    add_build_argument(parser)
    args = parser.parse_args()
    build_dir = args.build_dir

    if build_dir.is_symlink():
        raise ValueError(f"O diretório de build é um link simbólico: {build_dir}")
    resolved = build_dir.resolve()
    if resolved == PROJECT_ROOT or resolved in PROJECT_ROOT.parents:
        raise ValueError("O diretório de build não pode ser a raiz do projeto nem um ancestral dela.")
    if not build_dir.exists():
        print(f"Nada para limpar: {build_dir}")
        return 0
    if not build_dir.is_dir():
        raise ValueError(f"O caminho de build não é um diretório: {build_dir}")
    # A custom directory must be a CMake build generated for this project.
    if build_dir != DEFAULT_BUILD_DIR:
        read_build_description(build_dir)
    elif (build_dir / "project_description.json").exists():
        read_build_description(build_dir)

    shutil.rmtree(build_dir)
    print(f"Build removido: {build_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(entrypoint(main))
