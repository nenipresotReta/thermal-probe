"""Host regression for shared-newlib formatting, without the ESP32 SDK."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class FormatTests(unittest.TestCase):
    def test_application_uses_format_wrapper(self):
        raw_call = re.compile(r"\b(?:printf|snprintf|sprintf|vprintf|vsnprintf|vsprintf)\s*\(")
        for source in (ROOT / "src").glob("*.c"):
            if source.name == "format.c":
                continue
            with self.subTest(source=source.name):
                self.assertIsNone(raw_call.search(source.read_text()))

    def test_concurrent_formats(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            cc = os.environ.get("CC", "cc")
            flags = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-pthread",
                     "-Isrc", "-Itests/support"]
            subprocess.run(
                [cc, *flags, "-Dvsnprintf=test_vsnprintf", "-Dfwrite=test_fwrite",
                 "-c", "src/format.c", "-o", str(output / "format.o")],
                cwd=ROOT, check=True,
            )
            subprocess.run(
                [cc, *flags, "tests/format_test.c", str(output / "format.o"),
                 "-o", str(output / "format-test")],
                cwd=ROOT, check=True,
            )
            subprocess.run([str(output / "format-test")], check=True, timeout=30)


if __name__ == "__main__":
    unittest.main()
