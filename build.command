#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
if [[ "$(uname -s)" != Darwin ]]; then
  printf 'DesktopSnow: сборка приложения требует macOS 12+ и Xcode Command Line Tools.\n' >&2
  exit 1
fi
source ./sdk.command
SWIFTC="$(xcrun --sdk macosx --find swiftc)"
ARCH="$(uname -m)"
TARGET="$ARCH-apple-macosx12.0"
mkdir -p build DesktopSnow.app/Contents/MacOS DesktopSnow.app/Contents/Resources
printf 'Сборка и тестирование ядра (%s)…\n' "$ARCH"
CXX="$CXX" ./test.command
"$CXX" -std=c++17 -O2 -Wall -Wextra -Werror -pedantic "${SNOW_SDK_FLAGS[@]}" -target "$TARGET" -c Core/Snow.cpp -o build/Snow.o
"$SWIFTC" -sdk "$SDKROOT" -swift-version 5 -O -target "$TARGET" \
  -import-objc-header Core/Snow.h Sources/MetalParticles.swift Sources/main.swift build/Snow.o \
  -framework AppKit -framework CoreGraphics -framework Metal -framework MetalKit -framework QuartzCore -lc++ \
  -o DesktopSnow.app/Contents/MacOS/DesktopSnow
cp Info.plist DesktopSnow.app/Contents/Info.plist
# Runtime compilation uses Apple's Metal compiler through MTLDevice.makeLibrary.
# No Xcode-only offline metal tool dependency; resource failures trigger AppKit fallback.
cp Sources/Particles.metal DesktopSnow.app/Contents/Resources/Particles.metal
# Native macOS conversion; checked-in ICNS is an offline fallback.
ICON="Assets/DesktopSnow.icns"
if command -v iconutil >/dev/null 2>&1; then
  if iconutil -c icns Assets/DesktopSnow.iconset -o build/DesktopSnow.icns; then
    ICON="build/DesktopSnow.icns"
  else
    printf 'iconutil: используется готовая проверенная иконка.\n' >&2
  fi
fi
cp "$ICON" DesktopSnow.app/Contents/Resources/DesktopSnow.icns
# Ad-hoc signature only; no Apple Developer certificate or notarization.
codesign --force --deep --sign - DesktopSnow.app
printf '\nГотово: %s/DesktopSnow.app\nЗапуск: open DesktopSnow.app\n' "$PWD"
if [[ "${1:-}" == --run ]]; then open DesktopSnow.app; fi
