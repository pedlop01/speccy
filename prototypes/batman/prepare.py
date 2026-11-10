#!/usr/bin/env python3
"""Extract local tape assets once; no emulator is used by game.py."""
import argparse
import hashlib
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
TAPE_SHA256 = 'c0619e7f31047563bfb91ba504e002b1ef09c9955a1147b5bb4e10ff5951c427'
PACKAGES = ['allegro-5', 'allegro_image-5', 'libarchive', 'zlib', 'libcurl',
            'libcrypto', 'nlohmann_json', 'expat']


def prepare(tape):
    tape = tape.resolve()
    if hashlib.sha256(tape.read_bytes()).hexdigest() != TAPE_SHA256:
        raise ValueError('Use the checked bin/batman.tap release; this tape differs.')
    build = HERE / '.build'
    build.mkdir(exist_ok=True)
    flags = shlex.split(subprocess.check_output(
        ['pkg-config', '--cflags', '--libs', *PACKAGES], text=True))
    executable = build / 'extract-assets'
    compiler = shlex.split(os.environ.get('CXX', 'g++'))
    subprocess.run([*compiler, '-O2', '-std=c++11', str(HERE / 'extract_assets.cpp'),
                    str(ROOT / 'src/z80.cpp'), str(ROOT / 'src/ula.cpp'),
                    '-o', str(executable), *flags, '-pthread', '-lgif'], check=True)
    # Extraction is staged so a failed boot never replaces working assets.
    with tempfile.TemporaryDirectory(prefix='.assets-', dir=HERE) as temporary:
        result = subprocess.run([str(executable), str(tape), temporary], cwd=ROOT / 'bin',
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        (build / 'extraction.log').write_text(result.stdout)
        if result.returncode:
            raise RuntimeError(f'Asset extraction failed; see {build / "extraction.log"}')
        destination = HERE / 'assets'
        destination.mkdir(exist_ok=True)
        for path in Path(temporary).iterdir():
            shutil.copy2(path, destination / path.name)
        print(result.stdout.strip().splitlines()[-1])
    print(f'Ready: {sys.executable} {HERE / "game.py"}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tape', type=Path, default=ROOT / 'bin/batman.tap')
    args = parser.parse_args()
    try:
        prepare(args.tape)
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        parser.exit(1, f'{error}\nRequires the Linux emulator build dependencies in BUILD.md.\n')
