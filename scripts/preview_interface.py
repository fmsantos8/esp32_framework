#!/usr/bin/env python3
"""Render the real OLED interface as a standalone, offline HTML preview.

Usage: python3 scripts/preview_interface.py [--fonts] [--open]
Requires Python 3, CMake and a native C11 compiler (cc, gcc or clang), without ESP-IDF.
Re-run after changing the interface or fonts to refresh the snapshots.
"""

import argparse
import base64
import json
import os
from pathlib import Path
import shlex
import shutil
import struct
import subprocess
import sys
import tempfile
import webbrowser
import zlib

from _common import PROJECT_ROOT, project_path


SUPPORT_DIR = Path(__file__).resolve().parent / "interface_preview"
WIDTH, HEIGHT = 128, 64
SCENARIOS = [
    dict(id="normal", label="Leitura normal", temperature=24800, humidity=58300,
         uptime=3723000, status="ok", sample=True, attempted=True, fault=False,
         description="24,8 °C e 58,3 % de umidade. Última aquisição bem-sucedida; tempo ativo de 01:02:03."),
    dict(id="startup", label="Aguardando a primeira leitura", temperature=0, humidity=0,
         uptime=0, status="ok", sample=False, attempted=False, fault=False,
         description="Sem amostra válida: os valores aparecem como --,- e o diagnóstico mostra AGUARDANDO."),
    dict(id="crc", label="Erro de CRC · última leitura mantida", temperature=24800, humidity=58300,
         uptime=3728000, status="crc", sample=True, attempted=True, fault=True,
         description="A última amostra válida permanece na tela. O símbolo ! indica falha e o diagnóstico mostra ERRO CRC."),
    dict(id="timeout", label="Timeout · última leitura mantida", temperature=24800, humidity=58300,
         uptime=3730000, status="timeout", sample=True, attempted=True, fault=True,
         description="A aquisição excedeu o tempo de espera. O diagnóstico mostra TIMEOUT; os valores válidos são preservados."),
    dict(id="i2c", label="Erro de comunicação I2C", temperature=24800, humidity=58300,
         uptime=3732000, status="i2c", sample=True, attempted=True, fault=True,
         description="Falha de comunicação com o sensor, com o display disponível. O diagnóstico mostra ERRO I2C."),
    dict(id="unavailable", label="Sensor indisponível · sem amostra", temperature=0, humidity=0,
         uptime=5000, status="unavailable", sample=False, attempted=True, fault=True,
         description="O sensor ainda não forneceu uma amostra válida. O diagnóstico mostra INDISPONIVEL e as leituras ficam como --,-."),
    dict(id="negative", label="Temperatura negativa", temperature=-5300, humidity=42700,
         uptime=86400000 + 3723000, status="ok", sample=True, attempted=True, fault=False,
         description="Temperatura de −5,3 °C e umidade de 42,7 %. Após 24 horas, o tempo ativo inclui o número de dias."),
    dict(id="minimum", label="Valores mínimos", temperature=-45000, humidity=0,
         uptime=999, status="ok", sample=True, attempted=True, fault=False,
         description="Limites inferiores aceitos pela interface: −45,0 °C e 0,0 % de umidade."),
    dict(id="maximum", label="Valores máximos", temperature=130000, humidity=100000,
         uptime=9999999 * 86400000 + 86399000, status="ok", sample=True, attempted=True, fault=False,
         description="Limites superiores aceitos: 130,0 °C e 100,0 % de umidade, com o maior número de dias exibido sem saturação."),
    dict(id="rounding", label="Arredondamento para uma casa decimal", temperature=24950, humidity=99950,
         uptime=10000000 * 86400000, status="ok", sample=True, attempted=True, fault=False,
         description="24,950 °C vira 25,0 °C; 99,950 % vira 100,0 %. O contador de dias saturado aparece como 9999999+."),
]


def raster_png(pixels, width, height, scale=4):
    """Encode grayscale pixels with integer scaling, without interpolation."""
    rows = bytearray()
    for y in range(height):
        row = bytes(value for value in pixels[y * width:(y + 1) * width] for _ in range(scale))
        rows.extend((b"\0" + row) * scale)

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

    return (b"\x89PNG\r\n\x1a\n"
            + chunk(b"IHDR", struct.pack(">IIBBBBB", width * scale, height * scale, 8, 0, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))


def frame_pixels(framebuffer, width=WIDTH, height=HEIGHT):
    """Read pixels from the top-left region of an SSD1306 framebuffer."""
    return bytes(
        255 if framebuffer[(y // 8) * WIDTH + x] & (1 << (y % 8)) else 0
        for y in range(height) for x in range(width)
    )


def png(framebuffer, scale=4):
    """Decode SSD1306 page bytes into a grayscale PNG using only the stdlib."""
    return raster_png(frame_pixels(framebuffer), WIDTH, HEIGHT, scale)


def run_renderer(compiler, inputs="", arguments=()):
    """Build the native renderer through the root CMake configuration."""
    with tempfile.TemporaryDirectory(prefix="oled-preview-") as directory:
        executable = Path(directory) / "bin" / ("app_gui_preview.exe" if os.name == "nt" else "app_gui_preview")
        commands = [
            ["cmake", "-S", str(PROJECT_ROOT), "-B", directory,
             "-DFRAMEWORK_BUILD_HOST_TESTS=ON", "-DCMAKE_BUILD_TYPE=Debug",
             f"-DCMAKE_C_COMPILER={compiler[0]}", f"-DCMAKE_C_FLAGS={shlex.join(compiler[1:])}"],
            ["cmake", "--build", directory, "--target", "app_gui_preview", "--config", "Debug"],
        ]
        for command in commands:
            build = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            if build.returncode:
                raise ValueError(build.stdout.strip() or "Falha ao compilar a prévia da GUI.")
        result = subprocess.run([str(executable), *arguments], input=inputs, text=True, capture_output=True)
        if result.returncode:
            raise ValueError(result.stderr.strip() or "Falha ao renderizar a interface C.")
    return result.stdout


def render_frames(compiler):
    inputs = "".join(
        f"{screen} {case['temperature']} {case['humidity']} {int(case['sample'])} "
        f"{int(case['attempted'])} {case['status']} {case['uptime']}\n"
        for case in SCENARIOS for screen in (0, 1)
    )
    frames = [bytes.fromhex(line) for line in run_renderer(compiler, inputs).splitlines()]
    if len(frames) != len(SCENARIOS) * 2 or any(len(frame) != WIDTH * HEIGHT // 8 for frame in frames):
        raise ValueError("O renderizador produziu um framebuffer inválido.")
    return frames


def generate_font_preview(compiler, output_dir):
    """Export all decimal glyphs, their native metrics, and comparison strips."""
    names = ("SMALL", "MEDIUM", "LARGE")
    glyphs = {}
    for line in run_renderer(compiler, arguments=("--fonts",)).splitlines():
        font, digit, width, height, data = line.split()
        font, width, height = int(font), int(width), int(height)
        framebuffer = bytes.fromhex(data)
        key = (font, digit)
        if (font not in range(len(names)) or digit not in "0123456789" or len(digit) != 1
                or not 0 < width <= WIDTH or not 0 < height <= HEIGHT
                or len(framebuffer) != WIDTH * HEIGHT // 8 or key in glyphs):
            raise ValueError("O renderizador produziu um dígito inválido.")
        pixels = frame_pixels(framebuffer, width, height)
        if not any(pixels):
            raise ValueError("O renderizador produziu um dígito vazio.")
        glyphs[key] = dict(digit=digit, width=width, height=height, pixels=pixels)
    if set(glyphs) != {(font, str(digit)) for font in range(3) for digit in range(10)}:
        raise ValueError("A prévia deve conter os 30 dígitos das três fontes.")

    output_dir.mkdir(parents=True, exist_ok=True)
    fonts = []
    for font, name in enumerate(names):
        digits = [glyphs[font, str(digit)] for digit in range(10)]
        height = max(digit["height"] for digit in digits)
        cell_width = max(digit["width"] for digit in digits) + 8
        strip_width, strip_height = cell_width * 10 + 8, height + 8
        strip = bytearray(strip_width * strip_height)
        previews = []
        for index, digit in enumerate(digits):
            width = digit["width"]
            left = 4 + index * cell_width + (cell_width - width) // 2
            for row in range(digit["height"]):
                offset = (row + 4) * strip_width + left
                strip[offset:offset + width] = digit["pixels"][row * width:(row + 1) * width]
            encoded = base64.b64encode(raster_png(digit["pixels"], width, digit["height"], 1)).decode("ascii")
            previews.append({key: value for key, value in digit.items() if key != "pixels"})
            previews[-1]["image"] = "data:image/png;base64," + encoded
        filename = f"font-{name.lower()}.png"
        (output_dir / filename).write_bytes(raster_png(strip, strip_width, strip_height, 8))
        (output_dir / f"font-{name.lower()}-1x.png").write_bytes(raster_png(strip, strip_width, strip_height, 1))
        fonts.append(dict(name=name, height=height, digits=previews, strip=filename))
    template = (SUPPORT_DIR / "fonts.html").read_text(encoding="utf-8")
    page = output_dir / "index.html"
    page.write_text(template.replace("__FONT_DATA__", json.dumps(fonts, ensure_ascii=True)), encoding="utf-8")
    return page


def main():
    parser = argparse.ArgumentParser(description="Gera uma prévia fiel das duas telas do OLED no PC.")
    parser.add_argument("--output-dir", type=project_path,
                        help="Pasta de saída (padrão: build/interface_preview ou build/font_preview com --fonts).")
    parser.add_argument("--cc", default=os.environ.get("HOST_CC"),
                        help="Compilador C nativo; também pode ser definido em HOST_CC.")
    parser.add_argument("--open", action="store_true", help="Abre o HTML no navegador padrão.")
    parser.add_argument("--fonts", action="store_true", help="Gera os dígitos de 0 a 9 das três fontes da interface.")
    args = parser.parse_args()
    if args.output_dir is None:
        args.output_dir = PROJECT_ROOT / "build" / ("font_preview" if args.fonts else "interface_preview")
    compiler = shlex.split(args.cc) if args.cc else []
    if not compiler:
        native = next((path for name in ("cc", "gcc", "clang") if (path := shutil.which(name))), None)
        if not native:
            raise ValueError("Instale um compilador C nativo (gcc ou clang) ou informe --cc.")
        compiler = [native]
    if args.fonts:
        page = generate_font_preview(compiler, args.output_dir)
        print(f"Prévia das fontes: {page}")
        print("30 dígitos · SMALL, MEDIUM e LARGE · PNGs em escala 1× e 8×.")
        if args.open and not webbrowser.open(page.as_uri()):
            print("Abra o arquivo index.html acima no seu navegador.")
        return 0
    frames = render_frames(compiler)
    cases = []
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for index, case in enumerate(SCENARIOS):
        images = {}
        for screen, name in enumerate(("readings", "diagnostics")):
            picture = png(frames[index * 2 + screen])
            images[name] = "data:image/png;base64," + base64.b64encode(picture).decode("ascii")
            if index == 0:
                (args.output_dir / f"{name}.png").write_bytes(picture)
        cases.append({**case, **images})
    template = (SUPPORT_DIR / "index.html").read_text(encoding="utf-8")
    page = args.output_dir / "index.html"
    page.write_text(template.replace("__PREVIEW_DATA__", json.dumps(cases, ensure_ascii=True)), encoding="utf-8")
    print(f"Prévia gerada: {page}")
    print(f"{len(cases)} cenários · duas telas · renderização pelo código C do firmware.")
    if args.open and not webbrowser.open(page.as_uri()):
        print("Abra o arquivo index.html acima no seu navegador.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Erro: {error}", file=sys.stderr)
        raise SystemExit(1)
