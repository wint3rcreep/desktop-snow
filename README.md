# DesktopSnow

Independent MIT-licensed macOS menu-bar snow overlay, written in Swift/AppKit with a portable C++ simulation and optional hybrid Metal particle rendering. Includes falling snow, window-edge accumulation, snowballs, broom interaction, per-display controls and a bounded two-pass snowplow. Original included icons are covered by the MIT license; no proprietary application assets are used.

## Install (Apple Silicon)

```sh
brew install --cask wint3rcreep/tap/desktopsnow
open -a DesktopSnow
```

The initial 3.3.0 preview supports **arm64 only**, with a macOS 12 Monterey minimum deployment target. Native CI runs on macOS 15; macOS 12 runtime compatibility is not separately tested. Intel binaries are not distributed.

This preview is ad-hoc signed, **not Apple-notarized**. Gatekeeper can block first launch. Review the source/release and use the macOS Privacy & Security “Open Anyway” approval if you trust it; do not disable Gatekeeper or automatically strip quarantine. Grant Screen Recording permission to enable window geometry detection, then relaunch. The app queries window geometry; it does not record screen images. Use the menu-bar snow icon for settings and Quit. Interaction is opt-in; Escape returns to click-through.

```sh
brew update
brew upgrade --cask wint3rcreep/tap/desktopsnow
brew uninstall --cask wint3rcreep/tap/desktopsnow
```

Uninstall removes the app but deliberately leaves preferences and macOS permission decisions intact.

## Build and test on macOS

Install Xcode Command Line Tools (`xcode-select --install`), then:

```sh
./test.command
SNOW_TEST_FLAGS='-fsanitize=undefined -fno-sanitize-recover=all' ./test.command
./build.command
./DesktopSnow.app/Contents/MacOS/DesktopSnow --metal-self-test
open DesktopSnow.app
```

Build uses the selected macOS SDK and preserves an explicit `SDKROOT`. C++ and Swift compile natively; the production Metal shader is compiled by the runtime self-test. CI verifies the ad-hoc signature, packages using `ditto` and verifies the extracted executable/signature. Portable C++ tests also run on Linux (`bash test.command`).

## Preview limitations

A successful native build and Metal pipeline smoke test do not prove GUI quality. Interactive visual acceptance remains open: actual snow shedding during dragging/minimizing, Screen Recording permission flow, menu/click-through, Retina and multi-monitor layouts (including negative coordinates), Space/wake, fallback rendering, snowballs and tractor appearance. Window occlusion is rectangular and corner geometry heuristic; Dock/fullscreen handling is simplified. Snow is a bounded 2D area simulation, not a 3D volume; capacity limits and patch aggregation can reduce physical/spatial detail.

Version 3.3 corrects emission at the exported Bezier edge and lets detached snow patches fall immediately instead of remaining suspended during staged breakup. Production regression tests cover these behaviors.

## Distribution references

- [Homebrew cask cookbook](https://docs.brew.sh/Cask-Cookbook)
- [Maintaining a tap](https://docs.brew.sh/How-to-Create-and-Maintain-a-Tap)
- [Build and release runs](https://github.com/wint3rcreep/desktop-snow/actions)

License: [MIT](LICENSE).
