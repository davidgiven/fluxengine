#!/usr/bin/env bash
#
# resign-dmg.sh: unpack a dmg, ad-hoc sign the .app inside, and rebuild the dmg.
#
# Usage: ./resign-dmg.sh input.dmg [output.dmg]
# If output.dmg is omitted, the input file is replaced.
#
# Set SIGN_IDENTITY to use a real certificate instead of ad-hoc ("-"):
#   SIGN_IDENTITY="Developer ID Application: Your Name (TEAMID)" ./resign-dmg.sh in.dmg

set -euo pipefail

IN_DMG="${1:?Usage: $0 input.dmg [output.dmg]}"
OUT_DMG="${2:-$IN_DMG}"
IDENTITY="${SIGN_IDENTITY:--}"

[[ -f "$IN_DMG" ]] || { echo "Not found: $IN_DMG" >&2; exit 1; }

WORK="$(mktemp -d)"
MOUNT="$WORK/mount"
STAGE="$WORK/stage"
mkdir -p "$MOUNT" "$STAGE"

cleanup() {
  hdiutil detach "$MOUNT" -quiet -force 2>/dev/null || true
  rm -rf "$WORK"
}
trap cleanup EXIT

echo "==> Mounting $IN_DMG"
hdiutil attach "$IN_DMG" -mountpoint "$MOUNT" -nobrowse -readonly -quiet

# Reuse the original volume name
VOLNAME="$(diskutil info "$MOUNT" | sed -n 's/^ *Volume Name: *//p')"
VOLNAME="${VOLNAME:-$(basename "$IN_DMG" .dmg)}"

echo "==> Copying contents (volume: $VOLNAME)"
# ditto preserves symlinks (e.g. /Applications), xattrs and hidden files
ditto "$MOUNT" "$STAGE"
hdiutil detach "$MOUNT" -quiet

APP="$(find "$STAGE" -maxdepth 1 -name '*.app' -type d | head -n 1)"
[[ -n "$APP" ]] || { echo "No .app found in dmg" >&2; exit 1; }
echo "==> Found app: $(basename "$APP")"

# Remove any existing (possibly broken) signatures and quarantine flags
xattr -cr "$APP" || true
find "$APP" -name '_CodeSignature' -type d -prune -exec rm -rf {} + 2>/dev/null || true

echo "==> Signing nested binaries (identity: $IDENTITY)"
# Sign every Mach-O file (dylibs, jspawnhelper, java, launcher, ...) inside-out.
# Deepest paths first so nested code is signed before what contains it.
find "$APP" -type f -print0 \
  | while IFS= read -r -d '' f; do
      if file -b "$f" | grep -q 'Mach-O'; then
        printf '%s\n' "$f"
      fi
    done \
  | awk '{ print length($0) "\t" $0 }' | sort -rn | cut -f2- \
  | while IFS= read -r f; do
      codesign --force --sign "$IDENTITY" --timestamp=none "$f"
    done

echo "==> Signing app bundle"
codesign --force --sign "$IDENTITY" --timestamp=none "$APP"

echo "==> Verifying"
codesign --verify --deep --strict --verbose=2 "$APP"
codesign -dvv "$APP" 2>&1 | grep -E 'Signature|Identifier' || true

echo "==> Building dmg: $OUT_DMG"
TMP_DMG="$WORK/out.dmg"
hdiutil create -volname "$VOLNAME" -srcfolder "$STAGE" -ov -format UDZO -quiet "$TMP_DMG"
mv -f "$TMP_DMG" "$OUT_DMG"

echo "Done: $OUT_DMG"
