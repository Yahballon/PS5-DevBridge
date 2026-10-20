from pathlib import Path
import sys
root=Path(sys.argv[1])
s=(root/'src/platform/ps5/display_egl.cpp').read_text()
assert 'EGL_CONTEXT_MAJOR_VERSION_KHR,\n    3,\n    EGL_CONTEXT_MINOR_VERSION_KHR,\n    3,' in s
assert 'gl_major < 3 || (gl_major == 3 && gl_minor < 3)' in s
assert 'gl_major < 4' not in s
assert 'glClear(' in (root/'src/main.cpp').read_text()
print('PASS: native clear-based UI requests and accepts OpenGL 3.3')
