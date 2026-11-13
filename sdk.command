#!/bin/bash
# Source from build.command/test.command. Never replace an explicitly chosen SDK.
SNOW_SDK_FLAGS=()
if [[ "$(uname -s)" == Darwin ]]; then
  if ! command -v xcrun >/dev/null 2>&1; then
    printf 'Нет xcrun. Установите Command Line Tools: xcode-select --install\n' >&2
    exit 1
  fi
  if [[ -z "${SDKROOT:-}" ]]; then
    SDKROOT="$(xcrun --sdk macosx --show-sdk-path)" || exit 1
  fi
  if [[ ! -d "$SDKROOT" || ! -d "$SDKROOT/System/Library/Frameworks/AppKit.framework" ]]; then
    printf 'SDKROOT не указывает на macOS SDK с AppKit: %s\nПроверьте xcode-select -p и xcrun --sdk macosx --show-sdk-path. Исправьте SDKROOT явно.\n' "$SDKROOT" >&2
    exit 1
  fi
  export SDKROOT
  CXX="${CXX:-$(xcrun --sdk macosx --find clang++)}"
  SNOW_SDK_FLAGS=(-isysroot "$SDKROOT" -stdlib=libc++)
  if ! printf '#include <vector>\nint main(){std::vector<int> v;return int(v.size());}\n' | \
      "$CXX" -std=c++17 "${SNOW_SDK_FLAGS[@]}" -x c++ -fsyntax-only -; then
    printf '\nНе проходит проверка C++ <vector>. CXX=%s\nSDKROOT=%s\nПроверьте: xcode-select -p; xcrun --sdk macosx --find clang++; xcrun --sdk macosx --show-sdk-path\nВыберите согласованные Xcode/CLT и SDK. Не удаляйте CLT: сначала проверьте developer directory и переменные CXX/SDKROOT.\n' "$CXX" "$SDKROOT" >&2
    exit 1
  fi
else
  CXX="${CXX:-c++}"
fi
