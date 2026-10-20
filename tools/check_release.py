"""Audit the Git index; refuse unexpected files before source publication."""
from pathlib import Path
import json
import re
import subprocess
import sys

ALLOWED = set(['.github/workflows/ci.yml', '.gitignore', 'CMakeLists.txt', 'README.md', 'boards/lilygo/tdisplay_s3/Kconfig.tdisplay_s3', 'boards/lilygo/tdisplay_s3/board.cmake', 'boards/lilygo/tdisplay_s3/board.yml', 'boards/lilygo/tdisplay_s3/tdisplay_s3_procpu.dts', 'boards/lilygo/tdisplay_s3/tdisplay_s3_procpu_defconfig', 'build.sh', 'config/device.example.json', 'flash.sh', 'prj.conf', 'src/filter.c', 'src/filter.h', 'src/main.c', 'src/modbus_codec.c', 'src/modbus_codec.h', 'src/wireless.c', 'src/wireless.h', 'tests/filter_test.c', 'tests/modbus_test.c', 'tests/test_config.py', 'tools/check_multiclient.py', 'tools/check_wireless.py', 'tools/configure.py', 'west.yml', 'tools/check_release.py'])
ALLOWED.update([
    'src/display.c', 'src/display.h', 'src/drawing.h',
    'src/tdisplay_lcd.c', 'src/tdisplay_lcd.h',
    'src/http_request.c', 'src/http_request.h', 'tests/http_request_test.c',
])
root = Path(__file__).resolve().parents[1]
tracked = subprocess.check_output(['git', 'ls-files', '-z'], cwd=root).decode().split('\0')
failed = []
for name in filter(None, tracked):
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
print('PASS: %d approved text source files; empty config template; no private config or firmware images.' % len(list(filter(None, tracked))))
