#!/usr/bin/env python3
"""Prepare a project-local Windows SDK; never install packages on the host."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import tempfile
import urllib.request

REPO = 'https://repo.msys2.org/mingw/mingw64/'
ALLEGRO = ('https://github.com/liballeg/allegro5/releases/download/5.2.10.1/'
           'allegro-x86_64-w64-mingw32-gcc-14.2.0-posix-seh-static-5.2.10.0.zip')
ALLEGRO_SHA256 = '2590fe08df5f7f881ad8c280d2c11453671bbff282aed30237c47c70a44cdb16'
ALLEGRO_DEPS = ('https://github.com/liballeg/allegro5/releases/download/5.2.10.1/'
                'allegro_deps-x86_64-w64-mingw32-gcc-14.2.0-posix-seh-1.15.0.zip')
ALLEGRO_DEPS_SHA256 = '3c05fa14b15099187a81b73f85b36e4c06593f7d41fa67096a65fd1851731d19'
ROOTS = ['libarchive', 'zlib', 'curl', 'openssl', 'nlohmann-json', 'giflib', 'crt', 'headers', 'winpthreads']


def download(url, target, digest=None):
    if target.exists() and digest and hashlib.sha256(target.read_bytes()).hexdigest() == digest:
        return
    print('Downloading', target.name, flush=True)
    request = urllib.request.Request(url, headers={'User-Agent': 'Gomas Windows SDK setup'})
    temporary = target.with_suffix(target.suffix + '.part')
    try:
        with urllib.request.urlopen(request, timeout=60) as response, temporary.open('wb') as output:
            shutil.copyfileobj(response, output)
        if digest and hashlib.sha256(temporary.read_bytes()).hexdigest() != digest:
            raise RuntimeError('Checksum mismatch: ' + target.name)
        temporary.replace(target)
    finally:
        temporary.unlink(missing_ok=True)


def package_index(database):
    packages = {}
    with tarfile.open(database) as archive:
        for member in archive:
            if not member.name.endswith('/desc'):
                continue
            content = archive.extractfile(member).read().decode()
            fields = {block.splitlines()[0].strip('%'): block.splitlines()[1:]
                      for block in content.strip().split('\n\n')}
            packages[fields['NAME'][0]] = fields
    return packages


def resolve(packages):
    selected = {}

    def visit(dependency):
        name = re.split(r'[<>=]', dependency)[0]
        if name in selected:
            return
        if name not in packages:
            raise RuntimeError('Missing MSYS2 dependency: ' + name)
        package = packages[name]
        selected[name] = package
        for item in package.get('DEPENDS', []):
            visit(item)
    for root in ROOTS:
        visit('mingw-w64-x86_64-' + root)
    return list(selected.values())


def extract(archive, staging, prefix):
    # Package install scripts and metadata are not extracted or executed.
    listing = subprocess.check_output(['bsdtar', '-tf', str(archive)], text=True).splitlines()
    if not any(name.startswith(prefix + '/') for name in listing):
        return  # Runtime metapackages may contain metadata only.
    subprocess.run(['bsdtar', '-xf', str(archive), '-C', str(staging),
                    '--include', prefix + '/*'], check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path,
                        default=Path(__file__).resolve().parents[1] / '.windows-sdk')
    args = parser.parse_args()
    if not shutil.which('bsdtar'):
        parser.error('bsdtar is required (Arch package: libarchive)')
    root = args.directory.resolve()
    cache = root / 'downloads'
    cache.mkdir(parents=True, exist_ok=True)
    database = cache / 'mingw64.db'
    download(REPO + 'mingw64.db', database)
    index = package_index(database)
    packages = resolve(index)
    gcc = index['mingw-w64-x86_64-gcc']
    print(f'Preparing {len(packages)} MSYS2 packages plus the official Allegro SDK', flush=True)
    print('Package hashes are checked against the HTTPS repository database; '
          'this does not perform pacman signature verification.', flush=True)
    allegro = cache / 'allegro.zip'
    download(ALLEGRO, allegro, ALLEGRO_SHA256)
    allegro_deps = cache / 'allegro-deps.zip'
    download(ALLEGRO_DEPS, allegro_deps, ALLEGRO_DEPS_SHA256)
    gcc_archive = cache / gcc['FILENAME'][0]
    download(REPO + gcc_archive.name, gcc_archive, gcc['SHA256SUM'][0])
    with tempfile.TemporaryDirectory(prefix='staging-', dir=root) as temporary:
        staging = Path(temporary)
        extract(allegro, staging, 'allegro')
        sdk = staging / 'prefix'
        (staging / 'allegro').rename(sdk)
        for package in packages:
            filename = package['FILENAME'][0]
            if '/' in filename or '\\' in filename:
                raise RuntimeError('Invalid repository filename')
            archive = cache / filename
            download(REPO + filename, archive, package['SHA256SUM'][0])
            extract(archive, staging, 'mingw64')
        shutil.copytree(staging / 'mingw64', sdk, dirs_exist_ok=True)
        # MSYS2 metadata refers to /mingw64. Our isolated sysroot contains its
        # contents directly, so use / and let pkg-config apply the sysroot.
        for directory in [sdk / 'lib/pkgconfig', sdk / 'share/pkgconfig']:
            if directory.exists():
                for pc in directory.glob('*.pc'):
                    pc.write_text(pc.read_text().replace('/mingw64', ''))
        gcc_staging = staging / 'gcc-runtime'
        gcc_staging.mkdir()
        subprocess.run(['bsdtar', '-xf', str(gcc_archive), '-C', str(gcc_staging),
                        '--include', 'mingw64/include/c++/*',
                        '--include', 'mingw64/lib/libstdc++*.a',
                        '--include', 'mingw64/lib/libgcc*.a',
                        '--include', 'mingw64/lib/gcc/*', '--exclude', '*.dll'], check=True)
        shutil.copytree(gcc_staging / 'mingw64', sdk, dirs_exist_ok=True)
        extract(allegro_deps, staging, 'allegro_deps')
        (staging / 'allegro_deps').rename(sdk / 'allegro-deps')
        pcdir = sdk / 'lib/pkgconfig'
        pcdir.mkdir(parents=True, exist_ok=True)
        # Official Allegro Windows binary releases do not ship pkg-config files.
        # These entries deliberately link the DLL import libraries, NOT static
        # Allegro archives; the archive name refers to bundled third-party deps.
        for addon in ['', '_audio', '_dialog', '_font', '_primitives', '_image']:
            name = 'allegro' + addon
            (pcdir / (name + '-5.pc')).write_text(
                'prefix=/\nlibdir=${prefix}/lib\nincludedir=${prefix}/include\n'
                f'Name: {name}\nDescription: Official Allegro Windows SDK\n'
                'Version: 5.2.10.1\n' + ('Requires: allegro-5\n' if addon else '') +
                f'Libs: -L${{libdir}} -l{name}\nCflags: -I${{includedir}}\n')
        manifest = {'gcc_runtime_version': gcc['VERSION'][0], 'gcc_runtime_sha256': gcc['SHA256SUM'][0],
                    'allegro_deps': ALLEGRO_DEPS, 'allegro_deps_sha256': ALLEGRO_DEPS_SHA256,
                    'allegro': ALLEGRO, 'allegro_sha256': ALLEGRO_SHA256,
                    'packages': [{'name': p['NAME'][0], 'version': p['VERSION'][0],
                                  'sha256': p['SHA256SUM'][0]} for p in packages]}
        (sdk / 'gomas-sdk.json').write_text(json.dumps(manifest, indent=2) + '\n')
        destination = root / 'prefix'
        backup = root / 'previous'
        if backup.exists():
            shutil.rmtree(backup)
        if destination.exists():
            destination.rename(backup)
        try:
            sdk.rename(destination)
        except Exception:
            if backup.exists():
                backup.rename(destination)
            raise
        if backup.exists():
            shutil.rmtree(backup)
    print('Windows SDK ready:', destination, flush=True)


if __name__ == '__main__':
    main()
