#!/bin/bash
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
sdk=${PS5_PAYLOAD_SDK:-$root/app/.deps/native/ps5-payload-sdk}
[[ -d "$sdk" ]] || { echo "Set PS5_PAYLOAD_SDK to your payload SDK directory" >&2; exit 1; }
mkdir -p "$root/dist"
PS5_PAYLOAD_SDK="$sdk" make -C "$root/loader" -j4 > "$root/loader-build.log" 2>&1
cp "$root/loader/devbridge-loader.elf" "$root/dist/"
for name in controller screenshot-hold screenshot-files-probe screenshot-file-reader diagnostic-log-reader browser-circle-shell-v15; do
 PS5_PAYLOAD_SDK="$sdk" make -C "$root/helpers/$name" OUTPUT="$root/dist/$name.elf" -j4 >> "$root/loader-build.log" 2>&1
done
# (AI-assisted) Payload-only is the default; native TV console is optional.
if [[ "${1:-}" == "--with-app" ]]; then
 mkdir -p "$root/app/build"
 cp "$root/dist/devbridge-loader.elf" "$root/app/build/"
 make -C "$root/app" USE_CCACHE=0 BUILD_JOBS=4 app > "$root/app-build.log" 2>&1
cp "$root/app/dist/PPSA99337.zip" "$root/dist/PS5-DevBridge-v0.2.3.zip"

fi
