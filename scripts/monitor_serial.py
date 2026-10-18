#!/usr/bin/env python3
"""Receive HAL UART telemetry with timestamps, logging and USB reconnection."""

import argparse
import codecs
from contextlib import ExitStack
from datetime import datetime
import os
import sys
import time

from _common import PROJECT_ROOT, add_port_argument, entrypoint, positive_integer, project_path, serial_port


class Output:
    """Stream decoded text without waiting for a newline or splitting UTF-8 bytes."""

    def __init__(self, log_file, timestamps):
        self.log_file = log_file
        self.timestamps = timestamps
        self.line_start = True
        self.decoder = codecs.getincrementaldecoder("utf-8")(errors="replace")

    def write(self, data, final=False):
        text = self.decoder.decode(data, final=final)
        for part in text.splitlines(keepends=True):
            prefix = ""
            if self.line_start and self.timestamps:
                timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                prefix = f"[{timestamp}] "
            formatted = prefix + part
            sys.stdout.write(formatted)
            if self.log_file:
                self.log_file.write(formatted)
            self.line_start = part.endswith("\n")
        sys.stdout.flush()
        if self.log_file:
            self.log_file.flush()

    def disconnect(self):
        self.write(b"", final=True)
        if not self.line_start:
            sys.stdout.write("\n")
            if self.log_file:
                self.log_file.write("\n")
        self.line_start = True
        self.decoder.reset()


def run_monitor(args, port, output):
    import serial

    connected_once = False
    while True:
        # Set control lines before opening to avoid deliberately resetting the board.
        connection = serial.Serial(
            port=None, baudrate=args.baud, timeout=0.1, xonxoff=False,
            rtscts=False, dsrdtr=False,
        )
        connection.port = port
        connection.dtr = args.dtr
        connection.rts = args.rts
        try:
            connection.open()
        except (serial.SerialException, OSError) as error:
            connection.close()
            if not connected_once or args.no_reconnect:
                raise ValueError(f"Não foi possível abrir {port}: {error}") from error
            time.sleep(2)
            continue
        with connection:
            connected_once = True
            print(f"Conectado a {port} a {args.baud} baud. Ctrl+C encerra.", file=sys.stderr, flush=True)
            while True:
                try:
                    data = connection.read(256)
                except (serial.SerialException, OSError) as error:
                    output.disconnect()
                    if args.no_reconnect:
                        raise ValueError(f"Conexão serial interrompida: {error}") from error
                    print(f"Conexão interrompida: {error}. Reconectando...", file=sys.stderr, flush=True)
                    break
                output.write(data)
        time.sleep(2)


def main():
    parser = argparse.ArgumentParser(description="Monitora a telemetria da HAL UART sem depender do console do SDK.")
    add_port_argument(parser)
    parser.add_argument("baud_positional", nargs="?", type=positive_integer, metavar="BAUD")
    parser.add_argument(
        "-b", "--baud", type=positive_integer,
        help="Velocidade serial (padrão: FW_MONITOR_BAUD ou 115200).",
    )
    parser.add_argument("--no-log", action="store_true", help="Não salva a sessão em arquivo.")
    parser.add_argument(
        "--log-dir", type=project_path, default=PROJECT_ROOT / "logs",
        help="Pasta dos logs (padrão: logs, na raiz do projeto).",
    )
    parser.add_argument("--no-timestamps", action="store_true", help="Não adiciona horário às linhas recebidas.")
    parser.add_argument("--no-reconnect", action="store_true", help="Encerra se a conexão serial for perdida.")
    parser.add_argument("--dtr", action="store_true", help="Ativa DTR; por padrão, permanece desativado.")
    parser.add_argument("--rts", action="store_true", help="Ativa RTS; por padrão, permanece desativado.")
    args = parser.parse_args()
    if args.baud is not None and args.baud_positional is not None:
        parser.error("Informe a velocidade apenas uma vez: BAUD ou --baud.")
    args.baud = args.baud or args.baud_positional
    if args.baud is None:
        try:
            args.baud = positive_integer(os.environ.get("FW_MONITOR_BAUD", "115200"))
        except argparse.ArgumentTypeError as error:
            parser.error(str(error))
    port = serial_port(args)

    with ExitStack() as resources:
        log_file = None
        if not args.no_log:
            args.log_dir.mkdir(parents=True, exist_ok=True)
            log_path = args.log_dir / f"serial_{datetime.now():%Y%m%d_%H%M%S_%f}.log"
            log_file = resources.enter_context(log_path.open("x", encoding="utf-8"))
            print(f"Log: {log_path}", file=sys.stderr, flush=True)
        output = Output(log_file, timestamps=not args.no_timestamps)
        try:
            run_monitor(args, port, output)
        except KeyboardInterrupt:
            print("\nMonitor encerrado.", file=sys.stderr)
            return 0
        finally:
            output.disconnect()


if __name__ == "__main__":
    raise SystemExit(entrypoint(main))
