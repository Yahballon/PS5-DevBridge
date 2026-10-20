#!/usr/bin/env python3
"""Adapt the native UI template to the pinned OpenGL 3.3 SDK."""
from pathlib import Path
import re,sys

def patch(root):
    p=root/'src/platform/ps5/display_egl.cpp'
    s=p.read_text();old='EGL_CONTEXT_MAJOR_VERSION_KHR,\n    4,\n    EGL_CONTEXT_MINOR_VERSION_KHR,\n    6,'
    assert s.count(old)==1,'UI EGL context declaration changed'
    s=s.replace(old,'EGL_CONTEXT_MAJOR_VERSION_KHR,\n    3,\n    EGL_CONTEXT_MINOR_VERSION_KHR,\n    3,')
    gate='gl_major < 4 || (gl_major == 4 && gl_minor < 6)'
    assert s.count(gate)==1,'UI runtime version guard changed'
    p.write_text(s.replace(gate,'gl_major < 3 || (gl_major == 3 && gl_minor < 3)').replace('gl context below 4.6','gl context below 3.3'))

if __name__=='__main__':patch(Path(sys.argv[1]))
