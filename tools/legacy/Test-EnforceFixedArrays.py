"""Narrow literal-index/dimension lint. Not an Enforce compiler/type resolver."""
from pathlib import Path
import importlib.util
import re
import zlib

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
def scrub(text):
    return re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"',
                  lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), text)

def hazards(text):
    source = scrub(text)
    errors = []
    # Conservative file-local inference. Ambiguous names with multiple declared
    # sizes are checked only against the largest size (no false cross-scope hit).
    dimensions = {}
    declarations = set()
    for m in re.finditer(r'\bvector\s+(\w+)\s*(?:\[\s*(\d+)\s*\])?', source):
        dimensions.setdefault(m[1], set()).add(int(m[2]) if m[2] else 3)
        if m[2]: declarations.add(source.index('[', m.start(), m.end()))
    for m in re.finditer(r'(?<![\w.])(\w+)\s*\[\s*(-?\d+)\s*\]', source):
        if source.index('[', m.start(), m.end()) in declarations: continue
        sizes = dimensions.get(m[1])
        if sizes and not 0 <= int(m[2]) < max(sizes):
            errors.append((m.start(), f'{m[1]}[{m[2]}] outside {sorted(sizes)}'))
    types = {}
    for m in re.finditer(r'\b(TraceOBB|EntitySpawnParams)\s+(\w+)\b', source): types[m[2]] = m[1]
    for m in re.finditer(r'\b(\w+)\.(Mat|Transform)\s*\[\s*(-?\d+)\s*\]', source):
        size = {('TraceOBB','Mat'):3, ('EntitySpawnParams','Transform'):4}.get((types.get(m[1]),m[2]))
        if size and not 0 <= int(m[3]) < size:
            errors.append((m.start(), f'{m[1]}.{m[2]}[{m[3]}] outside vector[{size}]'))
    for m in re.finditer(r'\b(\w+)\.(Mat|Transform)\s*\[[^\]\n]+\]\s*\[\s*(-?\d+)\s*\]', source):
        if types.get(m[1]) in ('TraceOBB', 'EntitySpawnParams') and not 0 <= int(m[3]) < 3:
            errors.append((m.start(), 'matrix row vector component outside [0..2]'))
    for m in re.finditer(r'\b(?:GetWorldTransform|GetTransform|SetWorldTransform|SetTransform)\s*\(\s*(\w+)\.Mat\s*\)', source):
        if types.get(m[1]) == 'TraceOBB': errors.append((m.start(), '3-row Mat passed as 4-row entity transform'))
    return [(source[:offset].count('\n')+1, message) for offset,message in errors]

# Positive and negative fixtures protect the precise regression and nearby forms.
bad = ['TraceOBB box; box.Mat[3] = p;', 'TraceOBB b; b.Mat[-1] = p;',
       'EntitySpawnParams p; p.Transform[4] = v;', 'vector v; v[3] = 1;',
       'vector basis[3]; basis[3] = p;', 'vector transform[4]; transform[4] = p;',
       'TraceOBB box; entity.GetWorldTransform(box.Mat);',
       'TraceOBB box; box.Mat[0][3] = 1;',
       'EntitySpawnParams p; p.Transform[3][3] = 1;']
good = ['TraceOBB box; box.Mat[2] = p; box.Start = p;',
        'EntitySpawnParams p; p.Transform[3] = v;', 'vector v; v[2] = 1;',
        'vector matrix[4]; matrix[3] = p;', '// box.Mat[3]\nvector v;',
        'string message = "box.Mat[3]";', 'TraceOBB box; box.Mat[2][2] = 1;']
for fixture in bad: assert hazards(fixture), fixture
for fixture in good: assert not hazards(fixture), (fixture, hazards(fixture))

# Read actual installed generated Core declaration, not a remembered example.
spec = importlib.util.spec_from_file_location('installed_reader', TOOLS / 'Read-InstalledVanillaResource.py')
reader = importlib.util.module_from_spec(spec); spec.loader.exec_module(reader)
archive = Path(str(GAME_INSTALL / 'addons/core/data.pak'))
record = next(r for r in reader.index(archive) if r[0].lower().endswith('/traceobb.c'))
with archive.open('rb') as stream:
    stream.seek(record[1]); payload = stream.read(record[2])
if record[2] != record[3]: payload = zlib.decompress(payload)
assert re.search(rb'vector\s+Mat\[3\]', payload)
docs = Path(str(TOOLS_INSTALL / 'Workbench/docs/EnfusionScriptAPI/html/interfaceEntitySpawnParams.html')).read_text()
assert 'EntitySpawnParams.Transform[4]' in docs

files = list((ROOT/'Scripts').rglob('*.c'))
for file in files:
    errors = hazards(file.read_text(encoding='utf-8-sig'))
    assert not errors, (str(file.relative_to(ROOT)), errors)
print(f'FIXED ARRAY LINT PASS: {len(files)} scripts; {len(bad)+len(good)} regression fixtures; installed Mat[3]/Transform[4] verified')
print('Coverage: literal indexes of declared vectors/fixed vector arrays and typed Mat/Transform; known 4-row transform calls. Dynamic indexes, aliases and general overload resolution require Workbench.')
