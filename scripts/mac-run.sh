#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

EXE=build-mac/DxRenderDojo.exe
[ -f "$EXE" ] || { echo "$EXE 가 없습니다. scripts/mac-build.sh 로 먼저 빌드하세요."; exit 1; }

export WINEPREFIX="${WINEPREFIX:-$HOME/Library/Caches/DxRenderDojo/wineprefix}"
mkdir -p "$WINEPREFIX"
export WINEDEBUG="${WINEDEBUG:--all}"

WINE_BIN="$(command -v wine64 || command -v wine || true)"
[ -n "$WINE_BIN" ] || { echo "wine64 를 찾을 수 없습니다. Game Porting Toolkit 을 설치하세요."; exit 1; }

"$WINE_BIN" "$EXE"
