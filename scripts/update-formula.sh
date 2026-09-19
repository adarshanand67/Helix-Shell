#!/bin/bash

# Script to update the Homebrew formula with a new release
# Usage: ./update-formula.sh v1.0.0

set -e

if [ -z "$1" ]; then
    echo "❌ Error: Version tag required"
    echo "Usage: $0 v1.0.0"
    exit 1
fi

VERSION="$1"
FORMULA_FILE="Formula/helix-shell.rb"
REPO_URL="https://github.com/adarshanand67/Helix-Shell"
TARBALL_URL="${REPO_URL}/archive/refs/tags/${VERSION}.tar.gz"

echo "🔄 Updating Homebrew formula for HelixShell ${VERSION}"
echo ""

# Check if formula file exists
if [ ! -f "$FORMULA_FILE" ]; then
    echo "❌ Error: Formula file not found: $FORMULA_FILE"
    echo "Are you in the Helix-Shell repository root?"
    exit 1
fi

echo "📥 Downloading release tarball..."
echo "URL: $TARBALL_URL"
echo ""

# Download and calculate SHA256
echo "🔐 Calculating SHA256 hash..."
SHA256=$(curl -sL "$TARBALL_URL" | shasum -a 256 | awk '{print $1}')

if [ -z "$SHA256" ]; then
    echo "❌ Error: Failed to calculate SHA256"
    echo "Make sure the release $VERSION exists on GitHub"
    exit 1
fi

echo "✅ SHA256: $SHA256"
echo ""

# Update the formula
echo "📝 Updating formula..."

# Backup original
cp "$FORMULA_FILE" "${FORMULA_FILE}.bak"

# Update URL and SHA256
sed -i.tmp "s|url \".*\"|url \"${TARBALL_URL}\"|g" "$FORMULA_FILE"
sed -i.tmp "s|sha256 \".*\"|sha256 \"${SHA256}\"|g" "$FORMULA_FILE"
rm -f "${FORMULA_FILE}.tmp"

echo "✅ Formula updated successfully!"
echo ""

# Show diff
echo "📋 Changes:"
diff -u "${FORMULA_FILE}.bak" "$FORMULA_FILE" || true
echo ""

# Audit the formula
echo "🔍 Auditing formula..."
if command -v brew &> /dev/null; then
    brew audit --strict "$FORMULA_FILE" || {
        echo "⚠️  Warning: Formula audit found issues"
        echo "Review the output above and fix any problems"
    }
else
    echo "⚠️  Homebrew not found, skipping audit"
fi

echo ""
echo "✅ Done! Next steps:"
echo "1. Review the changes above"
echo "2. Test the formula: brew install --build-from-source helix-shell"
echo "3. Commit: git add $FORMULA_FILE && git commit -m 'Update to ${VERSION}'"
echo "4. Push: git push origin main"
echo ""
echo "To restore the backup: mv ${FORMULA_FILE}.bak $FORMULA_FILE"
