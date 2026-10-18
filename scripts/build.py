#!/usr/bin/env python3
"""Build the ESP32 firmware with an activated ESP-IDF 6.1 environment."""

import argparse
import time

from _common import PROJECT_ROOT, add_build_argument, entrypoint, idf_command, run_command


def main():
    parser = argparse.ArgumentParser(description="Compila o firmware com ESP-IDF 6.1.")
    add_build_argument(parser)
    parser.add_argument(
        "-D", "--define", action="append", default=[], metavar="NAME=VALUE",
        help="Definição do CMake; pode ser repetida, por exemplo -D SDKCONFIG=/tmp/sdkconfig.",
    )
    args = parser.parse_args()
    command = idf_command() + ["-C", str(PROJECT_ROOT), "-B", str(args.build_dir), "-D", "IDF_TARGET=esp32"]
    for definition in args.define:
        if "=" not in definition or not definition.split("=", 1)[0]:
            parser.error("Cada definição deve usar NAME=VALUE.")
        command.extend(["-D", definition])
    command.append("build")

    print(f"Compilando em {args.build_dir}...", flush=True)
    start = time.monotonic()
    result = run_command(command)
    status = "concluída" if result == 0 else "falhou"
    print(f"Compilação {status} ({time.monotonic() - start:.2f} s).", flush=True)
    return result


if __name__ == "__main__":
    raise SystemExit(entrypoint(main))
