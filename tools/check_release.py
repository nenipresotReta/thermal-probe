"""Audit the Git index; refuse unexpected files before source publication."""
from pathlib import Path
import json
import hashlib
import re
import subprocess
import sys

ALLOWED = set(['.github/workflows/ci.yml', '.gitignore', 'CMakeLists.txt', 'README.md', 'boards/lilygo/tdisplay_s3/Kconfig.tdisplay_s3', 'boards/lilygo/tdisplay_s3/board.cmake', 'boards/lilygo/tdisplay_s3/board.yml', 'boards/lilygo/tdisplay_s3/tdisplay_s3_procpu.dts', 'boards/lilygo/tdisplay_s3/tdisplay_s3_procpu_defconfig', 'build.sh', 'config/device.example.json', 'flash.sh', 'prj.conf', 'src/filter.c', 'src/filter.h', 'src/main.c', 'src/modbus_codec.c', 'src/modbus_codec.h', 'src/wireless.c', 'src/wireless.h', 'tests/filter_test.c', 'tests/modbus_test.c', 'tests/test_config.py', 'tools/check_multiclient.py', 'tools/check_wireless.py', 'tools/configure.py', 'west.yml', 'tools/check_release.py'])
ALLOWED.update([
    "LICENSE", "THIRD_PARTY_NOTICES.md", "THIRD_PARTY_NOTICES.zh-CN.md",
    "README.zh-CN.md", "CONTRIBUTING.zh-CN.md", "Kconfig", "headless.conf",
    "docs/interfaces.zh-CN.md", "docs/troubleshooting.zh-CN.md",
    "CONTRIBUTING.md", "docs/interfaces.md", "docs/troubleshooting.md",
    'src/display.c', 'src/display.h', 'src/drawing.h',
    'src/tdisplay_lcd.c', 'src/tdisplay_lcd.h',
    'src/http_request.c', 'src/http_request.h', 'tests/http_request_test.c',
    'src/format.c', 'src/format.h', 'tests/format_test.c', 'tests/test_format.py',
    'tests/support/zephyr/kernel.h',
])
# Owner-approved photos only; exact digests prevent arbitrary binary uploads.
APPROVED_IMAGES = {'docs/images/esp32-original.jpg': 'eb04db0dd55d7b4159017b2f5266c3ed77b441e9687f716b68a237eb1cd31910', 'docs/images/esp32-retouched.png': '1347b9c6f95d1dbd29a55ee639fa16fda3cccb7bdda0cdcd195b154df1520f2b', 'docs/images/max31865-original.jpg': 'd00ff2a9a977dac56362c3b11899109735bae1a3ef339f78b7cba83494c69da8', 'docs/images/max31865-retouched.png': '45f77006adfd252199f291603a143e3db8ad3e52dd11f34c7660b9426ad2d055'}
root = Path(__file__).resolve().parents[1]
tracked = subprocess.check_output(['git', 'ls-files', '-z'], cwd=root).decode().split('\0')
failed = []
for name in filter(None, tracked):
    if name in APPROVED_IMAGES:
        image_data = subprocess.check_output(['git', 'show', ':' + name], cwd=root)
        if hashlib.sha256(image_data).hexdigest() != APPROVED_IMAGES[name]:
            failed.append(name)
        continue
    if name not in ALLOWED:
        failed.append(name)
        continue
    data = subprocess.check_output(['git', 'show', ':' + name], cwd=root)
    try:
        text = data.decode('utf-8')
    except UnicodeError:
        failed.append(name)
        continue
    if b'\0' in data or re.search(r'gh[pousr]_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{20,}|-----BEGIN [A-Z ]+PRIVATE KEY-----', text):
        failed.append(name)
    if re.search(r'(?<![0-9])(?:10\.[0-9]+\.[0-9]+\.[0-9]+|192\.168\.[0-9]+\.[0-9]+)|(?:[0-9a-f]{2}:){5}[0-9a-f]{2}|' + '/Use' + r'rs/', text, re.I):
        failed.append(name)
    if name == 'config/device.example.json' and json.loads(text) != {'ssid': '', 'password': ''}:
        failed.append(name)
if failed:
    sys.exit('Publication refused; inspect files locally: ' + ', '.join(sorted(set(failed))))
print('PASS: %d approved source/documentation files (including hash-checked photos); empty config template; no private config or firmware images.' % len(list(filter(None, tracked))))
