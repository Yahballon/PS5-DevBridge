#!/usr/bin/env bash
# ProsperoStore - Build and run the read-only live catalog probe.
# Copyright (C) 2026 BlackBearReloaded
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$root"
bash tools/store-test.sh
clang++ -std=c++20 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc \
    host/catalog_probe.cpp host/http.cpp src/net/http.cpp src/net/curl_request.cpp src/catalog/catalog.cpp src/catalog/client.cpp \
    src/core/save_file.cpp build/store-tests/{monocypher_monocypher,monocypher_monocypher-ed25519,yyjson_yyjson}.o \
    -lcurl -o build/store-tests/catalog-probe
mkdir -p .local/cache
build/store-tests/catalog-probe .local/cache
build/store-tests/catalog-probe --memory
