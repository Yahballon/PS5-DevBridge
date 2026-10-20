#!/usr/bin/env bash
# (AI-assisted) Public LAN app with embedded key-free payload.
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$root/app/build"
cp "$root/dist/devbridge-loader.elf" "$root/app/build/"
make -C "$root/app" USE_CCACHE=0 BUILD_JOBS=4 APP_DEFINITIONS="GL_GLEXT_PROTOTYPES=1 DEVBRIDGE_PUBLIC_RELEASE=1" APP_ROOT_FILES=build/devbridge-loader.elf app
cp "$root/app/dist/PPSA99337.zip" "$root/dist/PS5-DevBridge-v0.2.2-release-app.zip"
