#!/usr/bin/env bash
# (AI-assisted) Redistributable native app: no embedded key or paired ELF.
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
make -C "$root/app" USE_CCACHE=0 BUILD_JOBS=4 APP_DEFINITIONS="GL_GLEXT_PROTOTYPES=1 DEVBRIDGE_PUBLIC_RELEASE=1" APP_ROOT_FILES= app
cp "$root/app/dist/PPSA99337.zip" "$root/dist/PS5-DevBridge-v0.2.0-release-app.zip"
