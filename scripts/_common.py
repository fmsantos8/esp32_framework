"""Shared paths, arguments and process helpers for the project scripts."""

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


PROJECT_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_BUILD_DIR = PROJECT_ROOT / "build"


def positive_integer(value):
    try:
        number = int(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("Use um número inteiro positivo.") from error
    if number <= 0:
        raise argparse.ArgumentTypeError("Use um número inteiro positivo.")
    return number


def project_path(value):
    path = Path(value).expanduser()
    if not path.is_absolute():
        path = PROJECT_ROOT / path
    return Path(os.path.abspath(path))


def add_build_argument(parser):
    parser.add_argument(
        "-B", "--build-dir", type=project_path, default=DEFAULT_BUILD_DIR,
        help="Diretório de build; caminhos relativos partem da raiz do projeto (padrão: build).",
    )


def add_port_argument(parser):
    parser.add_argument("port", nargs="?", help="Porta serial, por exemplo /dev/ttyUSB0 ou COM5.")
    parser.add_argument("-p", "--port", dest="port_option", help="Alternativa ao argumento posicional PORT.")


def serial_port(args):
    if args.port and args.port_option:
        raise ValueError("Informe a porta apenas uma vez: PORT ou --port.")
    configured = args.port_option or args.port or os.environ.get("FW_SERIAL_PORT") or os.environ.get("ESPPORT")
    if configured:
        return configured

    from serial.tools import list_ports

    ports = sorted(port.device for port in list_ports.comports() if port.vid is not None)
    if len(ports) == 1:
        return ports[0]
    if ports:
        raise ValueError("Mais de uma porta USB disponível: " + ", ".join(ports) + ". Informe --port.")
    raise ValueError("Nenhuma porta USB detectada. Informe --port ou defina FW_SERIAL_PORT/ESPPORT.")


def python_executable():
    environment = os.environ.get("IDF_PYTHON_ENV_PATH")
    if environment:
        relative = "Scripts/python.exe" if os.name == "nt" else "bin/python"
        executable = Path(environment) / relative
        if not executable.is_file():
            raise ValueError("IDF_PYTHON_ENV_PATH não contém um interpretador Python válido.")
        return str(executable)
    return sys.executable


def idf_command():
    sdk = os.environ.get("IDF_PATH")
    executable = str(Path(sdk) / "tools" / "idf.py") if sdk else shutil.which("idf.py")
    if not executable or not Path(executable).is_file():
        raise ValueError(
            "ESP-IDF não encontrado. Abra um terminal ESP-IDF 6.1 ou ative o ambiente "
            "com o script gerado pelo instalador (no EIM, activate_idf_<versão>.sh/.ps1). "
            "Para instalações tradicionais, use o export.sh/export.ps1 do SDK."
        )
    return [python_executable(), executable]


def read_build_description(build_dir):
    with (build_dir / "project_description.json").open(encoding="utf-8") as source:
        description = json.load(source)
    if not isinstance(description, dict) or not isinstance(description.get("project_path"), str):
        raise ValueError("Descrição de build inválida.")
    if Path(description["project_path"]).resolve() != PROJECT_ROOT:
        raise ValueError("O diretório de build pertence a outro projeto.")
    return description


def run_command(command, cwd=PROJECT_ROOT):
    try:
        result = subprocess.run([str(part) for part in command], cwd=cwd, check=False)
        return result.returncode if result.returncode >= 0 else 128 - result.returncode
    except KeyboardInterrupt:
        print("\nOperação interrompida.", file=sys.stderr)
        return 130


def entrypoint(main):
    try:
        return main()
    except KeyboardInterrupt:
        print("\nOperação interrompida.", file=sys.stderr)
        return 130
    except (OSError, ValueError, ImportError) as error:
        print(f"Erro: {error}", file=sys.stderr)
        if isinstance(error, ImportError):
            print("Execute com o Python do ambiente ESP-IDF 6.1, que inclui pyserial.", file=sys.stderr)
        return 1
