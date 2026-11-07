#!/usr/bin/env python3
"""Host-side regression checks for Windows packaging and cleanup."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from windows_bundle import cpp_imports


class WindowsBuildTests(unittest.TestCase):
    def test_cleanup_preserves_roms_sdk_and_personal_files(self):
        for target in ['clean-windows', 'clean']:
            with self.subTest(target=target), tempfile.TemporaryDirectory() as folder:
                root = Path(folder)
                for variant in ['windows', 'windows-static']:
                    release = root / variant
                    (release / 'roms').mkdir(parents=True)
                    (release / 'licenses').mkdir()
                    for name in ['gomas.exe', 'libstdc++-6.dll', 'curl-ca-bundle.crt', 'licenses/library.txt']:
                        (release / name).write_text('generated')
                    (release / 'roms/48.rom').write_text('keep')
                (root / 'sorteo.tap').write_text('keep')
                (root / 'gomas').write_text('native')
                (root / '.windows-sdk').mkdir()
                (root / '.windows-sdk/marker').write_text('keep')
                subprocess.run(['make', '-f', str(ROOT / 'bin/Makefile'), target],
                               cwd=root, check=True, stdout=subprocess.DEVNULL)
                for variant in ['windows', 'windows-static']:
                    release = root / variant
                    self.assertEqual((release / 'roms/48.rom').read_text(), 'keep')
                    for name in ['gomas.exe', 'libstdc++-6.dll', 'curl-ca-bundle.crt', 'licenses']:
                        self.assertFalse((release / name).exists())
                self.assertTrue((root / '.windows-sdk/marker').exists())
                self.assertTrue((root / 'sorteo.tap').exists())
                self.assertEqual((root / 'gomas').exists(), target == 'clean-windows')

    def test_cpp_imports_detects_the_reported_runtime_abi(self):
        expected = '_ZNSi5seekgESt4fposI9_MbstatetE'
        metadata = ('DLL Name: libstdc++-6.dll\n'
                    '    000bc740 <none> 0601 ' + expected + '\n\n'
                    'DLL Name: KERNEL32.dll\n'
                    '    000bc720 <none> 0001 GetLastError\n')
        imports = cpp_imports(metadata)
        self.assertEqual(imports['libstdc++-6.dll'], {expected})
        self.assertEqual(imports['kernel32.dll'], set())
        self.assertNotIn('_ZNSi5seekgESt4fposIiE', imports['libstdc++-6.dll'])


if __name__ == '__main__':
    unittest.main()
