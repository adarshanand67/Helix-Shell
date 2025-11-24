#!/bin/bash
set -e

# GitHub Release Creation Script for Helix Shell
# Usage: ./scripts/create-release.sh <version>
# Example: ./scripts/create-release.sh v1.2.0

VERSION=$1

if [ -z "$VERSION" ]; then
    echo "Error: Version argument required"
    echo "Usage: $0 <version>"
    echo "Example: $0 v1.2.0"
    exit 1
fi

# Validate version format (vX.Y.Z)
if ! [[ "$VERSION" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Error: Version must be in format vX.Y.Z (e.g., v1.2.0)"
    exit 1
fi

echo "Creating release for version: $VERSION"

# Check if gh CLI is installed
if ! command -v gh &> /dev/null; then
    echo "Error: GitHub CLI (gh) not found. Install it first:"
    echo "  macOS: brew install gh"
    echo "  Linux: https://github.com/cli/cli/blob/trunk/docs/install_linux.md"
    exit 1
fi

# Check if authenticated
if ! gh auth status &> /dev/null; then
    echo "Error: Not authenticated with GitHub CLI"
    echo "Run: gh auth login"
    exit 1
fi

# Create build directory for binaries
RELEASE_DIR="release-${VERSION}"
mkdir -p "$RELEASE_DIR"

echo "Building binaries for multiple platforms..."

# Build macOS binary (Apple Silicon)
echo "  → Building macOS (arm64)..."
rm -rf build && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
cd ..
cp build/helix "${RELEASE_DIR}/helix-macos-arm64"
chmod +x "${RELEASE_DIR}/helix-macos-arm64"

# Build macOS binary (Intel)
echo "  → Building macOS (x86_64)..."
rm -rf build && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=x86_64 ..
make -j$(sysctl -n hw.ncpu)
cd ..
cp build/helix "${RELEASE_DIR}/helix-macos-x86_64"
chmod +x "${RELEASE_DIR}/helix-macos-x86_64"

# Create universal binary for macOS
echo "  → Creating universal macOS binary..."
lipo -create \
    "${RELEASE_DIR}/helix-macos-arm64" \
    "${RELEASE_DIR}/helix-macos-x86_64" \
    -output "${RELEASE_DIR}/helix-macos-universal"
chmod +x "${RELEASE_DIR}/helix-macos-universal"

# Note: Linux and Windows builds would typically be done in CI/CD
echo ""
echo "Note: For Linux and Windows builds, use GitHub Actions workflow"
echo "      or Docker-based cross-compilation"

# Create tarball archives
echo "Creating release archives..."
cd "$RELEASE_DIR"
tar -czf "helix-${VERSION}-macos-arm64.tar.gz" helix-macos-arm64
tar -czf "helix-${VERSION}-macos-x86_64.tar.gz" helix-macos-x86_64
tar -czf "helix-${VERSION}-macos-universal.tar.gz" helix-macos-universal
cd ..

# Generate release notes
RELEASE_NOTES_FILE="${RELEASE_DIR}/release-notes.md"
echo "Generating release notes..."

cat > "$RELEASE_NOTES_FILE" <<EOF
# Helix Shell ${VERSION}

## Installation

### macOS
\`\`\`bash
# Download and extract
curl -L https://github.com/adarshanand67/Helix-Shell/releases/download/${VERSION}/helix-${VERSION}-macos-universal.tar.gz | tar xz

# Move to PATH
sudo mv helix-macos-universal /usr/local/bin/helix
chmod +x /usr/local/bin/helix

# Run
helix
\`\`\`

### Homebrew (Recommended for macOS)
\`\`\`bash
brew tap adarshanand67/helix-shell
brew install helix-shell
\`\`\`

### Docker
\`\`\`bash
docker pull adarshanand67/helixshell:${VERSION}
docker run -it adarshanand67/helixshell:${VERSION}
\`\`\`

## What's Changed

$(git log --oneline --pretty=format:"- %s" $(git describe --tags --abbrev=0 ${VERSION}^)..${VERSION} 2>/dev/null || echo "- Initial release")

## Features

- ✅ Full REPL with readline support
- ✅ Job control with fg/bg/jobs commands
- ✅ SIGCHLD signal handling for background jobs
- ✅ Pipeline support with multiple stages
- ✅ I/O redirection (<, >, >>, 2>, 2>>)
- ✅ Built-in commands (cd, pwd, export, history, exit)
- ✅ Tab autocompletion for commands and paths
- ✅ Colored prompt with Git integration
- ✅ Environment variable expansion
- ✅ Command history

## Downloads

| Platform | Architecture | Binary |
|----------|-------------|--------|
| macOS | Universal (ARM64 + x86_64) | helix-${VERSION}-macos-universal.tar.gz |
| macOS | ARM64 (Apple Silicon) | helix-${VERSION}-macos-arm64.tar.gz |
| macOS | x86_64 (Intel) | helix-${VERSION}-macos-x86_64.tar.gz |

**Full Changelog**: https://github.com/adarshanand67/Helix-Shell/compare/$(git describe --tags --abbrev=0 ${VERSION}^ 2>/dev/null || echo "v1.0.0")...${VERSION}
EOF

echo "Release notes generated at: $RELEASE_NOTES_FILE"

# Create GitHub release
echo ""
echo "Creating GitHub release..."

gh release create "$VERSION" \
    --title "Helix Shell ${VERSION}" \
    --notes-file "$RELEASE_NOTES_FILE" \
    "${RELEASE_DIR}/helix-${VERSION}-macos-arm64.tar.gz#macOS (ARM64)" \
    "${RELEASE_DIR}/helix-${VERSION}-macos-x86_64.tar.gz#macOS (Intel x86_64)" \
    "${RELEASE_DIR}/helix-${VERSION}-macos-universal.tar.gz#macOS (Universal Binary)"

echo ""
echo "✓ Release ${VERSION} created successfully!"
echo ""
echo "Release URL: https://github.com/adarshanand67/Helix-Shell/releases/tag/${VERSION}"
echo ""
echo "Next steps:"
echo "  1. Verify the release on GitHub"
echo "  2. Update Homebrew formula if needed"
echo "  3. Update Docker image tags"
echo "  4. Announce the release"

# Clean up
echo ""
read -p "Clean up build directory? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    rm -rf "$RELEASE_DIR" build
    echo "✓ Cleaned up temporary files"
fi
