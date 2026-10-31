"""Shared paths for preserved source/API/model tests; no game launches or writes."""
import os
from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[2]
ROOT = REPOSITORY / 'Addon'
TOOLS = Path(__file__).resolve().parent
GAME_INSTALL = Path(os.environ.get('BCM_REFORGER_DIR', str(Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)')) / 'Steam/steamapps/common/Arma Reforger')))
TOOLS_INSTALL = Path(os.environ.get('BCM_REFORGER_TOOLS_DIR', str(GAME_INSTALL.parent / 'Arma Reforger Tools')))
# No implicit lookup in the original developer's downloaded Workshop directory.
LOCAL_PACKAGE = Path(os.environ.get('BCM_PACKAGE_PATH', str(REPOSITORY / '.local/unset-package.data.pak')))
