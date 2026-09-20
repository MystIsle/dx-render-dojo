#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

[ -f build-mac/compile_commands.json ] || { echo "build-mac/compile_commands.json 이 없습니다. scripts/mac-build.sh 로 먼저 빌드하세요."; exit 1; }

LLVM_BIN="${LLVM_BIN:-$(brew --prefix llvm)/bin}"
[ -x "$LLVM_BIN/clang-tidy" ] || { echo "$LLVM_BIN/clang-tidy 가 없습니다. brew install llvm 으로 설치하세요."; exit 1; }

MINGW_ROOT="${MINGW_ROOT:-$(brew --prefix mingw-w64)/toolchain-x86_64}"
[ -d "$MINGW_ROOT" ] || { echo "$MINGW_ROOT 가 없습니다. brew install mingw-w64 로 설치하세요."; exit 1; }

FILES=("$@")
if [ ${#FILES[@]} -eq 0 ]; then
	while IFS= read -r File; do
		FILES+=("$File")
	done < <(find Source \( -name '*.h' -o -name '*.cpp' \) | sort)
fi

"$LLVM_BIN/clang-format" --dry-run -Werror "${FILES[@]}"

SOURCES=()
for File in "${FILES[@]}"; do
	case "$File" in
	*.cpp) SOURCES+=("$File") ;;
	esac
done

if [ ${#SOURCES[@]} -gt 0 ]; then
	"$LLVM_BIN/clang-tidy" -p build-mac --quiet --warnings-as-errors='*' \
		--extra-arg=--target=x86_64-w64-mingw32 \
		--extra-arg=--sysroot="$MINGW_ROOT" \
		--extra-arg=-std=c++20 \
		"${SOURCES[@]}"
fi

echo "검사 통과 : 서식 ${#FILES[@]}개, 네이밍 ${#SOURCES[@]}개"
