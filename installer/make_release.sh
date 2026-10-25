#!/bin/sh
# Build both cores and make the release zips for Windows and Linux
#   usage: installer/make_release.sh <version>   (e.g. v1.0.0)
#   output: dist/SpreMachineScope-<version>-{windows,linux}.zip
set -e
VER=${1:?usage: make_release.sh <version>}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
CLI=${ARDUINO_CLI:-arduino-cli}
PKG=${SPRESENSE_PKG:-$HOME/.arduino15/packages/SPRESENSE}
CORE=3.4.7
TOOLS=$PKG/tools/spresense-tools/$CORE
BUILD=$(mktemp -d)
trap 'rm -rf "$BUILD"' EXIT

$CLI compile -b SPRESENSE:spresense:spresense:Core=Main,Memory=896 --build-path "$BUILD/main" "$ROOT/Maincore"
$CLI compile -b SPRESENSE:spresense:spresense:Core=Sub1 --build-path "$BUILD/sub1" "$ROOT/Subcore1"

mkdir -p "$ROOT/dist"
for os in windows linux; do
  name=SpreMachineScope-$VER-$os
  d=$BUILD/$name
  mkdir -p "$d/firmware" "$d/tools" "$d/dsp"
  cp "$BUILD/main/Maincore.ino.spk" "$d/firmware/Maincore.spk"
  cp "$BUILD/sub1/Subcore1.ino.spk" "$d/firmware/Subcore1.spk"
  cp -r "$ROOT/Documents/config" "$d/config"
  cp "$ROOT/installer/README.txt" "$d/"
  cp "$PKG/hardware/spresense/$CORE/COPYING" "$d/tools/LICENSE-flash_writer.txt"
  cp "$PKG/hardware/spresense/$CORE/libraries/Audio/examples/dsp_installer/src_installer/SRC" "$d/dsp/SRC"
  cp "$PKG/hardware/spresense/$CORE/COPYING" "$d/dsp/LICENSE-SRC.txt"
  if [ $os = windows ]; then
    cp "$TOOLS/flash_writer/windows/flash_writer.exe" "$d/tools/"
    cp "$ROOT/installer/install.bat" "$d/"
  else
    cp "$TOOLS/flash_writer/linux/flash_writer" "$d/tools/"
    cp "$ROOT/installer/install.sh" "$d/"
    chmod +x "$d/install.sh" "$d/tools/flash_writer"
  fi
  rm -f "$ROOT/dist/$name.zip"
  (cd "$BUILD" && zip -qr "$ROOT/dist/$name.zip" "$name")
  echo "dist/$name.zip"
done
