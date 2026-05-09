#!/bin/bash
# Build and package Helix Shell.app into a DMG for the Homebrew cask.
# Usage: ./scripts/package-macos-app.sh v1.2.0
# Requires: create-dmg (brew install create-dmg) and a release binary in ./build/helix

set -e

VERSION="${1:?Usage: $0 vX.Y.Z}"
BARE="${VERSION#v}"
APP_DIR="macos-app/HelixShell.app"
BINARY="build/helix"
DEST_BINARY="$APP_DIR/Contents/MacOS/helix"
DMG_OUT="release/Helix-Shell-${VERSION}-macos.dmg"

if [ ! -f "$BINARY" ]; then
    echo "Build the project first: make build"
    exit 1
fi

mkdir -p release

# Embed the helix binary into the .app bundle
cp "$BINARY" "$DEST_BINARY"
chmod +x "$DEST_BINARY"

# Strip quarantine and ad-hoc sign so Gatekeeper doesn't kill it on first run
xattr -dr com.apple.quarantine "$DEST_BINARY" 2>/dev/null || true
codesign --force --deep --sign - "$APP_DIR" 2>/dev/null || true

# Update version in Info.plist
plutil -replace CFBundleVersion -string "$BARE" "$APP_DIR/Contents/Info.plist"
plutil -replace CFBundleShortVersionString -string "$BARE" "$APP_DIR/Contents/Info.plist"

# Build DMG
if ! command -v create-dmg &>/dev/null; then
    echo "create-dmg not found: brew install create-dmg"
    exit 1
fi

create-dmg \
    --volname "Helix Shell $BARE" \
    --window-pos 200 120 \
    --window-size 600 400 \
    --icon-size 100 \
    --app-drop-link 450 185 \
    "$DMG_OUT" \
    "$APP_DIR"

SHA=$(shasum -a 256 "$DMG_OUT" | awk '{print $1}')
echo ""
echo "DMG: $DMG_OUT"
echo "SHA256: $SHA"
echo ""
echo "Update Casks/helix-shell.rb:"
echo "  version \"$BARE\""
echo "  sha256 \"$SHA\""
