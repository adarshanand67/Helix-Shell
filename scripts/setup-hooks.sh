#!/bin/bash

# Git Hooks Setup Script
# Run this to install production Git hooks for the HelixShell project

set -e

echo "🔧 Setting up Git hooks for HelixShell..."

# Get repository root
REPO_ROOT=$(git rev-parse --show-toplevel 2>/dev/null || echo ".")
HOOKS_DIR="$REPO_ROOT/.git/hooks"

# Ensure hooks directory exists
if [ ! -d "$HOOKS_DIR" ]; then
    echo "❌ Error: .git/hooks directory not found"
    echo "   Make sure you're in a Git repository"
    exit 1
fi

cd "$REPO_ROOT"

# Backup existing hooks
echo "📦 Backing up existing hooks..."
for hook in pre-commit pre-push; do
    if [ -f "$HOOKS_DIR/$hook" ]; then
        cp "$HOOKS_DIR/$hook" "$HOOKS_DIR/$hook.backup.$(date +%Y%m%d_%H%M%S)"
        echo "   ✅ Backed up $hook"
    fi
done

# Copy hooks (they should already be in .git/hooks, but this ensures they're executable)
echo "📝 Setting up hooks..."
chmod +x "$HOOKS_DIR/pre-commit"
chmod +x "$HOOKS_DIR/pre-push"

# Verify hooks are executable
if [ -x "$HOOKS_DIR/pre-commit" ] && [ -x "$HOOKS_DIR/pre-push" ]; then
    echo "✅ Git hooks installed successfully!"
    echo ""
    echo "📋 Installed hooks:"
    echo "   • pre-commit:  Build checks, code quality, formatting"
    echo "   • pre-push:    Full build + test suite validation"
    echo ""
    echo "💡 To skip hooks (emergency only):"
    echo "   git commit --no-verify"
    echo "   git push --no-verify"
    echo ""
    echo "🎯 Test the hooks:"
    echo "   1. Make a small change to a file"
    echo "   2. git add <file>"
    echo "   3. git commit -m 'test'"
    echo "   (The pre-commit hook will run automatically)"
else
    echo "❌ Error: Failed to set up hooks"
    exit 1
fi
