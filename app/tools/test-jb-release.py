#!/usr/bin/env python3
# (AI-assisted) Offline GitHub release parsing and no-release cache regression tests.
from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parent.parent
out=r/'build/jb-preview';out.mkdir(parents=True,exist_ok=True);exe=out/'release-v51-test'
subprocess.run(['clang++-18','-std=c++20','-O1','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(r/'jb-backend/src'),str(r/'tests/test_jb_release.cpp'),str(r/'jb-backend/src/catalog.cpp'),str(r/'build/jb-json/libjb-json-host.a'),'-lcurl','-pthread','-o',str(exe)],check=True)
with tempfile.TemporaryDirectory() as folder:subprocess.run([str(exe),folder],check=True)
