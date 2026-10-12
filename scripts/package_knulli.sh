#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BINARY=${1:-build-arm64/living-village}
test -x "$BINARY" || { echo "Build the ARM64 binary first: ZIG=/path/to/zig ./scripts/build_arm64.sh" >&2; exit 1; }
readelf -h "$BINARY" | grep -q 'AArch64' || { echo "Refusing to package a non-ARM64 binary" >&2; exit 1; }
PACKAGE=dist/living-village-knulli-arm64
rm -rf "$PACKAGE"
mkdir -p "$PACKAGE/livingvillage/bin" "$PACKAGE/livingvillage/config" "$PACKAGE/livingvillage/saves" "$PACKAGE/livingvillage/logs"
cp "$BINARY" "$PACKAGE/livingvillage/bin/living-village"
cp config/controller.cfg "$PACKAGE/livingvillage/config/"
cp packaging/LivingVillage.sh "$PACKAGE/"
mkdir -p "$PACKAGE/livingvillage/images"
cp assets/cover.png "$PACKAGE/livingvillage/images/cover.png"
chmod 644 "$PACKAGE/livingvillage/images/cover.png"
cp README.md LICENSE "$PACKAGE/livingvillage/"
mkdir -p "$PACKAGE/livingvillage/licenses" "$PACKAGE/livingvillage/docs"
mkdir -p "$PACKAGE/livingvillage/scripts"
cp scripts/diagnose_knulli.sh scripts/register_art.sh scripts/compare_learning.sh "$PACKAGE/livingvillage/scripts/"
chmod 755 "$PACKAGE/livingvillage/scripts/register_art.sh" "$PACKAGE/livingvillage/scripts/compare_learning.sh"
chmod 755 "$PACKAGE/livingvillage/scripts/diagnose_knulli.sh"
cp docs/*.md "$PACKAGE/livingvillage/docs/"
cp vendor/SDL2/LICENSE.txt "$PACKAGE/livingvillage/licenses/SDL2.txt"
cp vendor/licenses/LLVM.txt "$PACKAGE/livingvillage/licenses/LLVM.txt"
cp vendor/licenses/Zig.txt "$PACKAGE/livingvillage/licenses/Zig.txt"
chmod 755 "$PACKAGE/LivingVillage.sh" "$PACKAGE/livingvillage/bin/living-village"
find "$PACKAGE" -type d -exec chmod 755 {} +
chmod 644 "$PACKAGE/livingvillage/config/controller.cfg" "$PACKAGE/livingvillage/README.md" "$PACKAGE/livingvillage/LICENSE" "$PACKAGE/livingvillage/licenses/SDL2.txt"
chmod 644 "$PACKAGE/livingvillage/licenses/LLVM.txt" "$PACKAGE/livingvillage/licenses/Zig.txt"
chmod 644 "$PACKAGE/livingvillage/docs/"*.md
tar -czf dist/living-village-knulli-arm64.tar.gz -C "$PACKAGE" .
echo "Ready: dist/living-village-knulli-arm64.tar.gz"
