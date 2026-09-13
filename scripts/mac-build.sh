#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

CONFIG="${1:-Debug}"

cmake -B build-mac -G Ninja \
	-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw-x86_64.cmake \
	-DCMAKE_BUILD_TYPE="$CONFIG"
cmake --build build-mac

echo "빌드 완료 : build-mac/DxRenderDojo.exe ($CONFIG)"
