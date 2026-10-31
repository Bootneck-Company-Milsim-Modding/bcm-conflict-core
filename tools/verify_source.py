"""Verify the frozen native source transfer. Standard library only; no native IO."""
import hashlib
import json
from pathlib import Path
import re
import sys


def main():
    root = Path(__file__).resolve().parents[1]
    baseline = json.loads((root / 'manifests/SourceBaseline.json').read_text(encoding='utf-8'))
    expected = {item['path']: item for item in baseline['files']}
    actual = {p.relative_to(root).as_posix(): p for p in (root / 'Addon').rglob('*')
              if p.is_file() and p.name != 'resourceDatabase.rdb' and '__pycache__' not in p.parts}
    problems = []
    for name in sorted(expected.keys() - actual.keys()):
        problems.append('MISSING ' + name)
    for name in sorted(actual.keys() - expected.keys()):
        problems.append('UNREVIEWED ADDON FILE ' + name)
    for name in sorted(expected.keys() & actual.keys()):
        payload = actual[name].read_bytes()
        if hashlib.sha256(payload).hexdigest() != expected[name]['sha256'] or len(payload) != expected[name]['bytes']:
            problems.append('CHANGED ' + name)
    project = (root / 'Addon/addon.gproj').read_text()
    mission = (root / 'Addon/Missions/Production/TB_CC_Arland.conf').read_text()
    if not re.search(r'GUID\s+"6A6A5973FA53A5B1"', project):
        problems.append('PROJECT GUID MISMATCH')
    if '{A4F3300000000000}Worlds/Production/Arland/TB_CC_Arland.ent' not in mission:
        problems.append('PRODUCTION WORLD REFERENCE MISMATCH')
    for forbidden in ('TestProfiles', 'ServerConfigs', 'TestConfigs', 'Documentation', 'Tools'):
        if (root / 'Addon' / forbidden).exists():
            problems.append('PRIVATE/DEVELOPMENT DIRECTORY INSIDE ADDON: ' + forbidden)
    if problems:
        print('\n'.join(problems))
        print('BASELINE DRIFT: review intentional edits; do not silently rewrite the manifest.')
        return 1
    print(f'SOURCE TRANSFER PASS: {len(expected)} native files match SHA-256 baseline; identity/mission/package-root checks pass.')
    print('NOT native compile, resource processing, Workshop package or runtime acceptance.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
