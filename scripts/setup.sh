#!/bin/bash

# HelixShell Development Environment Setup Script
# This script sets up all dependencies for building and testing the shell

set -e

echo "🔧 Setting up HelixShell development environment..."
echo ""

# Detect OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "📍 Detected: macOS"

    # Check for Homebrew
    if ! command -v brew &> /dev/null; then
        echo "❌ Homebrew not found!"
        echo "   Install from: https://brew.sh"
        exit 1
    fi

    echo "✅ Homebrew found"

    # Install dependencies
    echo ""
    echo "📦 Installing dependencies..."

    DEPS=("cmake" "pkg-config" "cppunit" "readline")

    for dep in "${DEPS[@]}"; do
        if brew list "$dep" &>/dev/null; then
            echo "   ✅ $dep already installed"
        else
            echo "   📥 Installing $dep..."
            brew install "$dep"
        fi
    done

elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "📍 Detected: Linux"

    # Check for apt (Debian/Ubuntu)
    if command -v apt-get &> /dev/null; then
        echo "📦 Installing dependencies via apt..."
        sudo apt-get update
        sudo apt-get install -y cmake build-essential pkg-config libcppunit-dev libreadline-dev
    # Check for yum (RedHat/CentOS)
    elif command -v yum &> /dev/null; then
        echo "📦 Installing dependencies via yum..."
        sudo yum install -y cmake gcc-c++ make pkg-config cppunit-devel readline-devel
    else
        echo "❌ Unsupported package manager"
        echo "   Please install manually: pkg-config, cppunit"
        exit 1
    fi

else
    echo "❌ Unsupported operating system: $OSTYPE"
    exit 1
fi

# Verify installations
echo ""
echo "🔍 Verifying installations..."

if command -v pkg-config &> /dev/null; then
    echo "   ✅ pkg-config: $(pkg-config --version)"
else
    echo "   ❌ pkg-config not found"
    exit 1
fi

if pkg-config --exists cppunit; then
    echo "   ✅ CppUnit: $(pkg-config --modversion cppunit)"
else
    echo "   ❌ CppUnit not found via pkg-config"
    exit 1
fi

# Setup Git hooks
echo ""
echo "🪝 Setting up Git hooks..."
if [ -f "scripts/setup-hooks.sh" ]; then
    chmod +x scripts/setup-hooks.sh
    ./scripts/setup-hooks.sh
else
    echo "   ⚠️  Git hooks setup script not found"
    echo "   Skipping hooks installation"
fi

# Test build
echo ""
echo "🔨 Testing build..."
if make build > /dev/null 2>&1; then
    echo "   ✅ Build test successful"
    make clean > /dev/null 2>&1
else
    echo "   ⚠️  Build test failed"
    echo "   This may be expected if source files are incomplete"
fi

echo ""
echo "✅ Setup complete!"
echo ""
echo "📋 Next steps:"
echo "   1. Build the project:  make build"
echo "   2. Run tests:          make test"
echo "   3. Run the shell:      ./build/hsh"
echo ""
echo "📚 Documentation:"
echo "   - Project README:      README.md"
echo "   - Git Hooks Guide:     scripts/README.md"
echo ""
