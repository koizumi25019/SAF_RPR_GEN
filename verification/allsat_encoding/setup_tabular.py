"""Fetch a pinned official enumeration solver and build only inside verification."""
import pathlib
import subprocess

HERE=pathlib.Path(__file__).resolve().parent
REVISION='ad4a071310581990b7834f1f076ccfb54d592bbf'
vendor=HERE/'vendor/tabularAllSAT'
if not vendor.exists():
    vendor.parent.mkdir(exist_ok=True)
    subprocess.run(['git','clone','https://github.com/giuspek/tabularAllSAT.git',str(vendor)],check=True)
    subprocess.run(['git','checkout','--detach',REVISION],cwd=vendor,check=True)
actual=subprocess.check_output(['git','rev-parse','HEAD'],cwd=vendor,text=True).strip()
assert actual==REVISION, f'Expected {REVISION}; existing checkout is {actual}'
assert not subprocess.check_output(['git','diff','HEAD','--name-only'],cwd=vendor,text=True).strip(), 'Upstream source modified'
(HERE/'runs').mkdir(exist_ok=True)
with (HERE/'runs/tabular_build.log').open('w') as log:
    subprocess.run(['./configure'],cwd=vendor/'cdcl-vsads',stdout=log,stderr=log,check=True)
    subprocess.run(['make','-j2'],cwd=vendor/'cdcl-vsads',stdout=log,stderr=log,check=True)
