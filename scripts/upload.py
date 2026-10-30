#!/usr/bin/env python3
"""Flash the existing ESP-IDF build and optionally open the serial monitor."""

import argparse
import json
import os
from pathlib import Path
import sys

from _common import (
    activate_idf_environment, add_build_argument, add_port_argument, entrypoint, positive_integer,
    python_executable, read_build_description, run_command, serial_port,
)


def main():
    parser = argparse.ArgumentParser(description="Grava o firmware já compilado e abre o monitor serial.")
    add_build_argument(parser)
    add_port_argument(parser)
    parser.add_argument(
        "-b", "--baud", type=positive_integer, default=os.environ.get("ESPBAUD", "460800"),
        help="Velocidade de gravação (padrão: ESPBAUD ou 460800).",
    )
    parser.add_argument(
        "--monitor-baud", type=positive_integer, default=os.environ.get("FW_MONITOR_BAUD", "115200"),
        help="Velocidade do monitor, independente da gravação (padrão: FW_MONITOR_BAUD ou 115200).",
    )
    parser.add_argument("--no-monitor", action="store_true", help="Encerra após gravar o firmware.")
    args = parser.parse_args()

    activate_idf_environment()
    python = python_executable()
    if python != sys.executable:
        # Port detection also needs pyserial from the ESP-IDF virtual environment.
        return run_command([python, Path(__file__).resolve(), *sys.argv[1:]])

    description = read_build_description(args.build_dir)
    if description.get("target") != "esp32":
        raise ValueError("O build informado não foi gerado para ESP32.")
    with (args.build_dir / "flasher_args.json").open(encoding="utf-8") as source:
        manifest = json.load(source)
    if not (args.build_dir / "flash_args").is_file():
        raise ValueError("flash_args não encontrado. Execute scripts/build.py primeiro.")
    if not isinstance(manifest, dict):
        raise ValueError("Manifesto de gravação inválido.")
    files = manifest.get("flash_files")
    extra = manifest.get("extra_esptool_args", {})
    if not isinstance(files, dict) or not files or not isinstance(extra, dict) or extra.get("chip") != "esp32":
        raise ValueError("Manifesto de gravação inválido para ESP32.")
    for name in files.values():
        if not isinstance(name, str) or not (args.build_dir / name).is_file():
            raise ValueError(f"Binário não encontrado: {name}. Execute scripts/build.py primeiro.")

    port = serial_port(args)
    command = [
        python, "-m", "esptool", "--chip", "esp32", "--port", port,
        "--baud", str(args.baud), "--before", extra.get("before", "default-reset"),
        "--after", extra.get("after", "hard-reset"),
    ]
    if not extra.get("stub", True):
        command.append("--no-stub")
    command.extend(["write-flash", "@flash_args"])

    print(f"Gravando em {port} a {args.baud} baud...", flush=True)
    result = run_command(command, cwd=args.build_dir)
    if result != 0:
        return result
    print("Gravação concluída.", flush=True)
    if args.no_monitor:
        return 0
    monitor = Path(__file__).with_name("monitor_serial.py")
    return run_command([python, monitor, "--port", port, "--baud", str(args.monitor_baud)])


if __name__ == "__main__":
    raise SystemExit(entrypoint(main))
