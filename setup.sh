#!/bin/bash

# Helix Shell Setup Script for macOS
# This script installs all dependencies required for building and testing the Helix Shell project

set -e  # Exit on any error

echo "🚀 Setting up Helix Shell development environment on macOS..."
echo "================================================================="

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check if Homebrew is installed
if ! command_exists brew; then
    echo "❌ Homebrew is not installed. Please install Homebrew first:"
    echo "   /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
    echo "   OR visit: https://brew.sh/"
    exit 1
fi

echo "✅ Homebrew is installed"

# Update Homebrew
echo "📦 Updating Homebrew..."
brew update

# Install required packages
PACKAGES=("cmake" "pkg-config" "cppunit")

for package in "${PACKAGES[@]}"; do
    if brew list "$package" >/dev/null 2>&1; then
        echo "✅ $package is already installed"
    else
        echo "📦 Installing $package..."
        brew install "$package"
    fi
done

# Verify installations
echo ""
echo "🔍 Verifying installations..."

if ! command_exists cmake; then
    echo "❌ CMake not found in PATH after installation"
    exit 1
fi

if ! command_exists pkg-config; then
    echo "❌ pkg-config not found in PATH after installation"
    exit 1
fi

# Check if CppUnit is properly installed
if ! pkg-config --libs cppunit >/dev/null 2>&1; then
    echo "❌ CppUnit not properly installed"
    exit 1
fi

echo "✅ All dependencies verified successfully!"

echo ""
echo "🎉 Setup complete! You can now build and test the Helix Shell:"
echo ""
echo "   mkdir build"
echo "   cd build"
echo "   cmake .."
echo "   make"
echo "   ./hsh_tests  # Run tests"
echo "   ./hsh        # Run the shell"
echo ""
echo "For development with tests (when running tests):"
echo "   cd build/tests"
echo "   ./hsh_tests"
echo ""
echo "Happy coding! 🚀"
