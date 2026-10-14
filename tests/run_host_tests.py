#!/usr/bin/env python3
"""Compile SDK-free headers and exercise the ESP32 port with a simulated SDK."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
headers = sorted((root / "hal/interface").rglob("*.h")) + [root / "memory/framework_memory.h"]
public_includes = ["-I", str(root / "hal/interface"), "-I", str(root / "memory")]
checks = 0
for header in headers:
    contents = header.read_text()
    assert "/**<" not in contents, header
    for declaration in re.finditer(r"hal_status_t\s+(\w+)\s*\(([^;]*)\);", contents):
        parameters = [] if declaration[2].strip() == "void" else [
            re.search(r"(\w+)\s*$", parameter)[1] for parameter in declaration[2].split(",")]
        prefix = contents[:declaration.start()]
        documentation = prefix[prefix.rfind("/**"):]
        assert parameters == re.findall(r"@param(?:\[[^\]]+\])?\s+(\w+)", documentation), declaration[1]
    include_root = root / "memory" if header.parent == root / "memory" else root / "hal/interface"
    include_name = header.relative_to(include_root).as_posix()
    source = f'#include "{include_name}"\n#include "{include_name}"\nint main(void) {{ return 0; }}\n'
    for compiler, language, standard in (("cc", "c", "c99"), ("cc", "c", "c11"), ("c++", "c++", "c++11")):
        subprocess.run([compiler, *public_includes, "-x", language, "-std=" + standard, "-Wall", "-Wextra", "-Werror",
                        "-pedantic-errors", "-fsyntax-only", "-"], input=source, text=True, check=True)
        checks += 1

with tempfile.TemporaryDirectory(prefix="tcc-host-tests-") as directory:
    directory = Path(directory)
    implementation = directory / "implementation.c"
    implementation.write_text('#include "common/hal_status.h"\n#include "gpio/hal_gpio.h"\n'
                              'hal_status_t hal_gpio_write(hal_gpio_t *gpio, bool level) '
                              '{(void)gpio; (void)level; return HAL_STATUS_OK;}\n')
    consumer = directory / "consumer.cpp"
    consumer.write_text('#include "common/hal_status.h"\n#include "gpio/hal_gpio.h"\n'
                        'int main() {return hal_gpio_write(nullptr, true) != HAL_STATUS_OK;}\n')
    subprocess.run(["cc", *public_includes, "-std=c99", "-c", str(implementation), "-o", str(directory / "implementation.o")], check=True)
    subprocess.run(["c++", *public_includes, "-std=c++11", str(consumer), str(directory / "implementation.o"), "-o", str(directory / "consumer")], check=True)
    subprocess.run([str(directory / "consumer")], check=True)
    port = root / "hal/ports/esp32-wroom-32"
    sources = [root / "memory/framework_memory.c", port / "port_resources.c", port / "port_info.c",
               *sorted(port.glob("*/hal_*.c")), root / "bsp/bsp_esp32.c",
               root / "tests/fakes/fake_sdk.c", root / "tests/test_contracts.c"]
    executable = directory / "contracts"
    command = ["cc", "-std=c11", "-O2", "-fstrict-aliasing", "-Wall", "-Wextra", "-Werror", "-pedantic-errors",
               "-DFRAMEWORK_MEMORY_CAPACITY=1024"]
    for include in (root / "tests/fakes", root / "hal/interface", root / "memory", root / "bsp", port):
        command += ["-I", str(include)]
    subprocess.run(command + [str(path) for path in sources] + ["-o", str(executable)], check=True)
    for scenario in ("arena", "ports", "exhaustion", "failed-construction", "bsp"):
        subprocess.run([str(executable), scenario], check=True)
print(f"PASS: {len(headers)} self-contained headers, {checks} C99/C11/C++11 checks, C/C++ linkage.")
