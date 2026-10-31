"""Run preserved tests with explicit machine-local SDK paths. Never launches Reforger."""
import argparse
import os
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reforger-dir', type=Path, default=os.environ.get('BCM_REFORGER_DIR'))
    parser.add_argument('--workbench-dir', type=Path, default=os.environ.get('BCM_REFORGER_TOOLS_DIR'))
    parser.add_argument('--package', type=Path, help='Optional specific staged data.pak; no implicit Workshop package')
    parser.add_argument('--suite', choices=('all', 'persistence', 'campaign'), default='all')
    args = parser.parse_args()
    if not args.reforger_dir:
        parser.error('Supply --reforger-dir or BCM_REFORGER_DIR for this computer')
    game = args.reforger_dir.expanduser().resolve()
    for relative in ('addons/core/data.pak', 'addons/data/data007.pak', 'addons/data/resourceDatabase.rdb'):
        if not (game / relative).is_file():
            parser.error(f'Installed native resource missing: {game / relative}')
    workbench = (args.workbench_dir or game.parent / 'Arma Reforger Tools').expanduser().resolve()
    if args.suite in ('campaign', 'all'):
        api_doc = workbench / 'Workbench/docs/EnfusionScriptAPI/html/interfaceEntitySpawnParams.html'
        if not api_doc.is_file():
            parser.error(f'Installed Tools API documentation missing; supply --workbench-dir: {api_doc}')
    if args.package and not args.package.is_file():
        parser.error('--package must identify an existing staged archive')
    repo = Path(__file__).resolve().parents[1]
    env = os.environ.copy()
    env['BCM_REFORGER_DIR'] = str(game)
    env['BCM_REFORGER_TOOLS_DIR'] = str(workbench)
    env['PYTHONDONTWRITEBYTECODE'] = '1'
    if args.package:
        env['BCM_PACKAGE_PATH'] = str(args.package.resolve())
    else:
        env.pop('BCM_PACKAGE_PATH', None)
    tests = []
    if args.suite in ('all', 'persistence'):
        tests.append('Test-CampaignAdministration.py')
    if args.suite in ('all', 'campaign'):
        tests.append('Test-EndgamePressure.py')
    if args.suite == 'all':
        tests.append('Test-ConstructionPersistenceNativeBoundary.py')
    for name in tests:
        print(f'RUN {name}', flush=True)
        result = subprocess.run([sys.executable, '-B', str(repo / 'tools/legacy' / name)], cwd=repo, env=env)
        if result.returncode:
            return result.returncode
    print('STATIC/API/MODEL SUITES PASS. Workbench/resource/dedicated/runtime gates remain unperformed.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
