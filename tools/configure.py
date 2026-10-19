"""Validate private configuration and create a local-only C header."""
import json
import os
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

def prepare(root=ROOT):
    destination = root / 'local/wifi_credentials.h'
    try:
        cfg = json.loads((root / 'config/device.json').read_text())
        ssid, password = cfg['ssid'], cfg['password']
        if not isinstance(ssid, str) or not isinstance(password, str):
            raise ValueError()
        if not 1 <= len(ssid.encode('utf-8')) <= 32:
            raise ValueError()
        if not 8 <= len(password.encode('utf-8')) <= 63:
            raise ValueError()
        if any(ord(c) < 32 or ord(c) == 127 for c in ssid + password):
            raise ValueError()
        # Byte escapes preserve UTF-8 without C universal-character ambiguities.
        def literal(value):
            return '"' + ''.join('\\%03o' % b for b in value.encode('utf-8')) + '"'
        content = '#define WIFI_SSID ' + literal(ssid) + '\n#define WIFI_PASSWORD ' + literal(password) + '\n'
    except (OSError, ValueError, KeyError, TypeError):
        destination.unlink(missing_ok=True)
        raise ValueError('Configuration required: copy config/device.example.json to config/device.json; fill a 2.4 GHz WPA2 SSID (1–32 UTF-8 bytes) and password (8–63 UTF-8 bytes). Values are never printed.') from None
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_suffix('.tmp')
    fd = os.open(temporary, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    os.chmod(temporary, 0o600)
    with os.fdopen(fd, 'w') as stream:
        stream.write(content)
    temporary.replace(destination)
    print('Local configuration validated; credentials hidden.')

if __name__ == '__main__':
    try:
        prepare()
    except ValueError as exc:
        sys.exit(str(exc))
