"""Run with Python on Linux/WSL; no heater access."""
from pathlib import Path
import subprocess
import tempfile

launcher = Path(__file__).resolve().parents[1] / 'controller/run_read_check.sh'
subprocess.run(['bash', '-n', str(launcher)], check=True)
with tempfile.TemporaryDirectory(prefix='cta_read_check_') as directory:
    root = Path(directory)
    (root / 'logs').mkdir()
    old_log = root / 'logs/cta_events.csv'
    old_log.write_text('old test data')
    for _ in range(2):
        subprocess.run(['bash', str(launcher), '/bin/true'], cwd=root, check=True)
    assert old_log.read_text() == 'old test data'
    runs = list((root / 'logs').glob('read_check_*'))
    assert len(runs) == 2
    assert all((run / 'cta_controller.log').exists() for run in runs)
print('Launcher checks passed')
