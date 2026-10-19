import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('configure', Path(__file__).resolve().parents[1] / 'tools/configure.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class ConfigTests(unittest.TestCase):
    def test_missing_and_blank_remove_stale_header(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'local').mkdir()
            (root / 'local/wifi_credentials.h').write_text('stale')
            with self.assertRaises(ValueError): module.prepare(root)
            self.assertFalse((root / 'local/wifi_credentials.h').exists())
            (root / 'config').mkdir()
            (root / 'config/device.json').write_text('{"ssid":"", "password":""}')
            with self.assertRaises(ValueError): module.prepare(root)

    def test_valid_and_escaped_utf8(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'config').mkdir()
            cfg = {'ssid': '测试"network', 'password': 'example\\password'}
            (root / 'config/device.json').write_text(json.dumps(cfg))
            module.prepare(root)
            p = root / 'local/wifi_credentials.h'
            self.assertEqual(p.stat().st_mode & 0o777, 0o600)
            self.assertIn('\\346\\265\\213', p.read_text())
            self.assertNotIn('example', p.read_text())

    def test_invalid_types_lengths_and_controls(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'config').mkdir()
            for cfg in [{}, {'ssid': 1, 'password': 'examplepass'}, {'ssid':'x'*33,'password':'examplepass'}, {'ssid':'x','password':'short'}, {'ssid':'x\n','password':'examplepass'}]:
                (root / 'config/device.json').write_text(json.dumps(cfg))
                with self.assertRaises(ValueError): module.prepare(root)
