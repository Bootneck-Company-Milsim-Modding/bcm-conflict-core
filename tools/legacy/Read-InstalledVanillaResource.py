"""Read selected text resources from installed PAC1 archives; never writes them.

Audit helper only. Rejects unsupported records/compression rather than returning
guessed content. Output includes archive, resource path and stored sizes.
"""
import argparse
import pathlib
import re
import struct
import zlib


def index(archive):
    with archive.open('rb') as stream:
        if stream.read(4) != b'FORM':
            raise ValueError('not a FORM archive')
        stream.read(4)
        if stream.read(4) != b'PAC1':
            raise ValueError('unsupported archive type')
        while True:
            tag = stream.read(4)
            if not tag:
                raise ValueError(f'no FILE index in {archive}')
            length = struct.unpack('>I', stream.read(4))[0]
            if tag == b'FILE':
                data = stream.read(length)
                break
            stream.seek(length, 1)
    cursor = 0
    records = []

    def walk(parent):
        nonlocal cursor
        kind, name_length = data[cursor:cursor + 2]
        cursor += 2
        name = data[cursor:cursor + name_length].decode('utf-8')
        cursor += name_length
        path = f'{parent}/{name}'.strip('/')
        if kind == 0:
            count = struct.unpack_from('<I', data, cursor)[0]
            cursor += 4
            for _ in range(count):
                walk(path)
        elif kind == 1:
            offset, stored, unpacked = struct.unpack_from('<III', data, cursor)
            cursor += 24
            records.append((path, offset, stored, unpacked))
        else:
            raise ValueError(f'unsupported index record {kind} at {cursor}')

    walk('')
    if cursor != len(data):
        raise ValueError(f'index boundary mismatch: {cursor}/{len(data)}')
    return records


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', type=pathlib.Path)
    parser.add_argument('names', nargs='+')
    parser.add_argument('--list', action='store_true')
    parser.add_argument('--lines', help='Inclusive line range, START:END')
    parser.add_argument('--grep', help='Regex; print matches with context')
    parser.add_argument('--context', type=int, default=5)
    parser.add_argument('--binary-header', action='store_true')
    parser.add_argument('--strings', help='Regex matched against printable strings in binary resources')
    args = parser.parse_args()
    wanted = [name.lower() for name in args.names]
    found = 0
    for archive in sorted(args.directory.glob('*.pak')):
        for path, offset, stored, unpacked in index(archive):
            if not any(name in path.lower() for name in wanted):
                continue
            found += 1
            print(f'RESOURCE {path} ARCHIVE {archive.name} STORED {stored} UNPACKED {unpacked}')
            if args.list:
                continue
            with archive.open('rb') as stream:
                stream.seek(offset)
                payload = stream.read(stored)
            if stored != unpacked:
                payload = zlib.decompress(payload)
            if len(payload) != unpacked:
                raise ValueError(f'payload size mismatch for {path}')
            if args.binary_header:
                print(payload[:64].hex(' '))
                continue
            if args.strings:
                for match in re.finditer(rb'[ -~]{4,}', payload):
                    value = match.group().decode('ascii')
                    if re.search(args.strings, value, re.IGNORECASE):
                        print(f'BYTE {match.start()}: {value}')
                continue
            lines = payload.decode('utf-8').replace('\r', '').splitlines()
            selected = range(len(lines))
            if args.lines:
                start, end = map(int, args.lines.split(':'))
                selected = range(max(0, start - 1), min(end, len(lines)))
            elif args.grep:
                selected = set()
                for number, line in enumerate(lines):
                    if re.search(args.grep, line):
                        selected.update(range(max(0, number - args.context), min(len(lines), number + args.context + 1)))
                selected = sorted(selected)
            for number in selected:
                print(f'{number + 1}: {lines[number]}')
    if not found:
        raise SystemExit('No matching installed resources')


if __name__ == '__main__':
    main()
