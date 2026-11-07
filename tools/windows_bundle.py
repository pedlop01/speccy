#!/usr/bin/env python3
"""Copy recursively imported SDK DLLs beside the Windows executable."""
import argparse
from pathlib import Path
import re
import shutil
import ssl
import subprocess

SYSTEM = set('kernel32 msvcrt ucrtbase user32 gdi32 advapi32 comdlg32 shell32 '
             'ole32 oleaut32 winmm ws2_32 crypt32 bcrypt secur32 shlwapi imm32 '
             'dwmapi ntdll psapi version comctl32 dinput8 dsound setupapi iphlpapi '
             'wldap32 dnsapi normaliz windowscodecs winhttp wintrust rpcrt4 '
             'dbghelp opengl32 powrprof userenv usp10 avrt hid cfgmgr32 oleacc '
             'msimg32 netapi32 xmllite'.split())


def cpp_imports(metadata):
    imports = {}
    for match in re.finditer(r'DLL Name:\s*(\S+)(.*?)(?=DLL Name:|\Z)', metadata, re.S):
        imports[match[1].lower()] = set(re.findall(
            r'^\s+[0-9a-fA-F]+[^\n]*?\s(_Z\S+)\s*$', match[2], re.M))
    return imports


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--prefix', default='x86_64-w64-mingw32')
    parser.add_argument('--assert-static', action='store_true', help='Reject all non-system DLL imports')
    args = parser.parse_args()
    output = args.exe.resolve().parent
    available = {p.name.lower(): p for p in (args.sdk / 'bin').glob('*.dll')}
    pending, seen = [args.exe], set()
    while pending:
        binary = pending.pop()
        metadata = subprocess.check_output([args.prefix + '-objdump', '-p', str(binary)], text=True)
        required_cpp = cpp_imports(metadata)
        for name in re.findall(r'DLL Name:\s*(\S+)', metadata):
            key = name.lower()
            if key in seen:
                continue
            seen.add(key)
            if args.assert_static:
                if key[:-4] in SYSTEM or key.startswith(('api-ms-', 'ext-ms-')):
                    continue
                raise RuntimeError('Static EXE imports a non-system DLL: ' + name)
            source = available.get(key)
            if source is None and (key[:-4] in SYSTEM or key.startswith(('api-ms-', 'ext-ms-'))):
                continue
            if source is None:
                location = subprocess.check_output([args.prefix + '-g++', '-print-file-name=' + name], text=True).strip()
                source = Path(location)
                if location == name or not source.is_file():
                    raise RuntimeError('Missing runtime DLL: ' + name)
            required = required_cpp.get(key, set())
            if required:
                exports = subprocess.check_output([args.prefix + '-objdump', '-p', str(source)], text=True)
                exported = set(re.findall(r'\b(_Z\S+)', exports))
                missing = required - exported
                if missing:
                    raise RuntimeError('C++ runtime ABI mismatch: ' + name + ' does not export ' + sorted(missing)[0])
            destination = output / name
            shutil.copy2(source, destination)
            pending.append(destination)
            print('Bundled', name, flush=True)
    # curl/OpenSSL's MSYS2 installation path does not exist on Windows. Ship a
    # optional public trust bundle in addition to Windows' native CA store.
    certificates = ssl.get_default_verify_paths().cafile
    if certificates:
        shutil.copyfile(certificates, output / 'curl-ca-bundle.crt')
    else:
        raise RuntimeError('No host CA bundle available for Windows HTTPS')
    licenses = args.sdk / 'share/licenses'
    if licenses.exists():
        shutil.copytree(licenses, output / 'licenses', dirs_exist_ok=True)
    for pattern in ['*LICENSE*', '*license*', '*COPYING*']:
        for path in args.sdk.glob(pattern):
            if path.is_file():
                shutil.copy2(path, output / path.name)
    print('Windows files ready in', output)


if __name__ == '__main__':
    main()
