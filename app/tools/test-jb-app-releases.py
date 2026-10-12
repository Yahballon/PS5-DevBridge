#!/usr/bin/env python3
# (AI-assisted) Offline GitHub app update regression tests.
from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent.parent;out=r/'build/jb-preview';out.mkdir(parents=True,exist_ok=True);exe=out/'app-release-test'
subprocess.run(['clang++-18','-std=c++20','-O1','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(r/'jb-backend/src'),str(r/'tests/test_jb_app_releases.cpp'),str(r/'jb-backend/src/app_releases.cpp'),str(r/'jb-backend/src/catalog.cpp'),str(r/'build/jb-json/libjb-json-host.a'),'-lcurl','-pthread','-o',str(exe)],check=True)
subprocess.run([str(exe)],check=True)
