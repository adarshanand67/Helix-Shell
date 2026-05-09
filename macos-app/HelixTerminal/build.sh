#!/bin/bash
# Build HelixTerminal.app — native macOS terminal emulator for Helix Shell
# Usage: ./build.sh [--release]
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT_DIR="$SCRIPT_DIR/../../build"
APP_DIR="$OUT_DIR/HelixShell.app"
CONTENTS="$APP_DIR/Contents"
MACOS="$CONTENTS/MacOS"
RESOURCES="$CONTENTS/Resources"

mkdir -p "$MACOS" "$RESOURCES"

FLAGS="-O -whole-module-optimization"
[[ "$1" == "--release" ]] || FLAGS="-g"

echo "Compiling HelixTerminal..."
swiftc $FLAGS \
    -sdk "$(xcrun --show-sdk-path)" \
    -target arm64-apple-macosx12.0 \
    -framework AppKit \
    -framework Foundation \
    Sources/main.swift \
    Sources/TerminalController.swift \
    -o "$MACOS/helix-terminal"

# Copy the helix binary into the bundle
HELIX_BIN="$SCRIPT_DIR/../../build/helix"
if [ -f "$HELIX_BIN" ]; then
    cp "$HELIX_BIN" "$MACOS/helix"
    chmod +x "$MACOS/helix"
    xattr -dr com.apple.quarantine "$MACOS/helix" 2>/dev/null || true
    codesign --force --sign - "$MACOS/helix" 2>/dev/null || true
fi

# Copy resources
cp -f "$SCRIPT_DIR/../icon.iconset/icon_512x512@2x.png" "$RESOURCES/" 2>/dev/null || true
if [ -f "$SCRIPT_DIR/../HelixShell.app/Contents/Resources/AppIcon.icns" ]; then
    cp "$SCRIPT_DIR/../HelixShell.app/Contents/Resources/AppIcon.icns" "$RESOURCES/AppIcon.icns"
fi

# Write Info.plist
cat > "$CONTENTS/Info.plist" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleIdentifier</key>         <string>com.adarshanand67.helix-shell</string>
  <key>CFBundleName</key>               <string>Helix Shell</string>
  <key>CFBundleDisplayName</key>        <string>Helix Shell</string>
  <key>CFBundleExecutable</key>         <string>helix-terminal</string>
  <key>CFBundleIconFile</key>           <string>AppIcon</string>
  <key>CFBundleVersion</key>            <string>1.0.0</string>
  <key>CFBundleShortVersionString</key> <string>1.0.0</string>
  <key>CFBundlePackageType</key>        <string>APPL</string>
  <key>LSMinimumSystemVersion</key>     <string>12.0</string>
  <key>NSHighResolutionCapable</key>    <true/>
  <key>NSPrincipalClass</key>           <string>NSApplication</string>
</dict>
</plist>
EOF

codesign --force --deep --sign - "$APP_DIR" 2>/dev/null || true

echo "Built: $APP_DIR"
echo "Run:   open $APP_DIR"
