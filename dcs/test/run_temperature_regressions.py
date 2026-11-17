"""Build the controller and run temperature regressions without serial hardware.

Run on Linux from any directory: python3 test/run_temperature_regressions.py
Only the regression executables are run; the controller is compiled, not started.
"""

from pathlib import Path
import subprocess
import tempfile


def main():
    root = Path(__file__).resolve().parents[1]
    common = ["g++", "-std=c++11", "-pthread", "-I" + str(root),
              "-I" + str(root / "third_party")]
    with tempfile.TemporaryDirectory(prefix="cta_temperature_tests_") as temp:
        build = Path(temp)
        library = build / "libcta_temperature_test.so"
        sources = sorted((root / "cta2045").rglob("*.cpp"))
        subprocess.run(common + ["-shared", "-fPIC"] + [str(path) for path in sources]
                       + ["-o", str(library)], check=True)
        controllers = [str(path) for path in sorted((root / "controller").glob("*.cpp"))
                       if path.name != "main.cpp"]
        link = [str(library), "-Wl,-rpath," + str(build)]
        for name in ("main", "device_information_privacy", "present_temperature_parser"):
            source = root / ("controller" if name == "main" else "test") / (name + ".cpp")
            executable = build / name
            subprocess.run(common + [str(source)] + controllers + link
                           + ["-o", str(executable)], check=True)
            if name != "main":
                work = build / (name + "_logs")
                work.mkdir()
                subprocess.run([str(executable)], cwd=work, check=True)
            print("PASS:", "controller build" if name == "main" else name, flush=True)


if __name__ == "__main__":
    main()
