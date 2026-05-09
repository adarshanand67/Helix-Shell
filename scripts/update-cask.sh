#!/bin/bash
# Update the Helix Shell cask and binary formula to a new release.
# Usage: ./update-cask.sh v1.2.0

set -e

VERSION="${1:?Usage: $0 vX.Y.Z}"
BARE="${VERSION#v}"
REPO_URL="https://github.com/adarshanand67/Helix-Shell"

patch_sha() {
    local file="$1" url="$2" key="$3"
    local sha
    sha=$(curl -fsSL "$url" | shasum -a 256 | awk '{print $1}')
    [ -z "$sha" ] && { echo "Failed to fetch $url"; exit 1; }
    # Replace sha256 :no_check placeholder with real hash for the given arch/key
    sed -i.bak "s|sha256 :no_check.*# $key|sha256 \"$sha\" # $key|g" "$file"
    rm -f "$file.bak"
    echo "  $key: $sha"
}

echo "Updating to $VERSION..."

# --- Formula/helix-shell.rb (source build) ---
F_SRC="Formula/helix-shell.rb"
TARBALL_URL="${REPO_URL}/archive/refs/tags/${VERSION}.tar.gz"
SRC_SHA=$(curl -fsSL "$TARBALL_URL" | shasum -a 256 | awk '{print $1}')
sed -i.bak \
    -e "s|url \".*archive/refs/tags/.*\"|url \"${TARBALL_URL}\"|" \
    -e "s|sha256 \"[a-f0-9]*\"|sha256 \"${SRC_SHA}\"|" \
    "$F_SRC"
rm -f "$F_SRC.bak"
echo "helix-shell.rb (source): $SRC_SHA"

# --- Formula/helix-shell-bin.rb (pre-built binaries) ---
F_BIN="Formula/helix-shell-bin.rb"
sed -i.bak "s|version \"[^\"]*\"|version \"${BARE}\"|" "$F_BIN"
rm -f "$F_BIN.bak"
echo "helix-shell-bin.rb: version → $BARE (sha256 :no_check kept — update manually after release)"

# --- Casks/helix-shell.rb ---
F_CASK="Casks/helix-shell.rb"
sed -i.bak "s|version \"[^\"]*\"|version \"${BARE}\"|" "$F_CASK"
rm -f "$F_CASK.bak"
echo "helix-shell cask: version → $BARE (sha256 :no_check kept — update after DMG is uploaded)"

echo ""
echo "Done. Review diffs:"
git diff Formula/ Casks/ || true
echo ""
echo "Next:"
echo "  1. brew audit Formula/helix-shell.rb"
echo "  2. git add Formula/ Casks/ && git commit -m 'Update to $VERSION'"
echo "  3. git push"
