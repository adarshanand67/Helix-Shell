#!/bin/bash
#
# HelixShell Installer
#
# This script installs HelixShell (hsh) on your system.
# It downloads the appropriate binary for your platform from GitHub releases.
#
# Usage:
#   curl -fsSL https://raw.githubusercontent.com/adarshanand67/Helix-Shell/main/install.sh | bash
#   or
#   wget -qO- https://raw.githubusercontent.com/adarshanand67/Helix-Shell/main/install.sh | bash
#
# Binary installation location:
#   Linux/macOS: ~/.local/bin/helix (or /usr/local/bin/helix with sudo)
#   The script will add ~/.local/bin to PATH if it's not already there.

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Repository information
REPO="adarshanand67/Helix-Shell"
LATEST_RELEASE_URL="https://api.github.com/repos/$REPO/releases/latest"

# Print colored output
info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

success() {
    echo -e "${GREEN}✅ $1${NC}"
}

warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

error() {
    echo -e "${RED}❌ $1${NC}"
}

# Detect platform
detect_platform() {
    case "$(uname -s)" in
        Linux*)
            echo "linux"
            ;;
        Darwin*)
            echo "macos"
            ;;
        *)
            error "Unsupported platform: $(uname -s)"
            exit 1
            ;;
    esac
}

# Detect architecture
detect_arch() {
    case "$(uname -m)" in
        x86_64|amd64)
            echo "x86_64"
            ;;
        arm64|aarch64)
            echo "arm64"
            ;;
        *)
            error "Unsupported architecture: $(uname -m)"
            exit 1
            ;;
    esac
}

# Download file with progress
download() {
    local url="$1"
    local output="$2"

    info "Downloading from $url"

    if command -v curl >/dev/null 2>&1; then
        curl -fsSL --progress-bar "$url" -o "$output"
    elif command -v wget >/dev/null 2>&1; then
        wget -q --show-progress "$url" -O "$output"
    else
        error "Neither curl nor wget is available"
        exit 1
    fi
}

# Get latest release information from GitHub API
get_latest_release() {
    if ! command -v jq >/dev/null 2>&1; then
        warning "jq not found, using fallback method"
        # Fallback: try to get the latest version from the releases page
        local version
        version=$(curl -s "https://github.com/$REPO/releases/latest" | grep -oP '(?<=/releases/tag/v)[^"]*' | head -1)
        if [ -z "$version" ]; then
            error "Could not determine latest version"
            exit 1
        fi
        echo "$version"
        return
    fi

    local response
    response=$(curl -fsSL "$LATEST_RELEASE_URL")

    if [ $? -ne 0 ]; then
        error "Failed to fetch release information from GitHub"
        exit 1
    fi

    echo "$response"
}

# Find download URL for platform
find_binary_url() {
    local release_info="$1"
    local platform="$2"
    local arch="$3"

    if command -v jq >/dev/null 2>&1; then
        # Extract download URLs from release assets
        local binary_name="helix-${platform}-${arch}"
        local url
        url=$(echo "$release_info" | jq -r ".assets[] | select(.name | startswith(\"$binary_name\")) | .browser_download_url" | head -1)

        if [ -z "$url" ] || [ "$url" = "null" ]; then
            error "No binary found for $platform-$arch"
            echo "$release_info" | jq -r '.assets[].name'
            exit 1
        fi

        echo "$url"
    else
        # Fallback without jq - this is more fragile
        warning "Using fallback download URL detection (less reliable)"
        echo "https://github.com/$REPO/releases/latest/download/helix-$platform-$arch"
    fi
}

# Install binary
install_binary() {
    local binary_path="$1"
    local install_path="$2"

    if [ -w "$(dirname "$install_path")" ] || [ "$USE_SUDO" = "true" ]; then
        if [ "$USE_SUDO" = "true" ]; then
            info "Installing to $install_path (requires sudo)"
            sudo cp "$binary_path" "$install_path"
            sudo chmod +x "$install_path"
        else
            info "Installing to $install_path"
            cp "$binary_path" "$install_path"
            chmod +x "$install_path"
        fi
    else
        error "Cannot write to $install_path. Try running with sudo or use -l for local installation."
        exit 1
    fi
}

# Setup PATH for local installation
setup_path() {
    local bin_dir="$HOME/.local/bin"

    if [[ ":$PATH:" != *":$bin_dir:"* ]]; then
        info "Adding $bin_dir to PATH"

        # Determine shell and config file
        local shell_config
        if [ -n "$ZSH_VERSION" ]; then
            shell_config="$HOME/.zshrc"
        elif [ -n "$BASH_VERSION" ]; then
            shell_config="$HOME/.bashrc"
            # Check for .bash_profile on macOS
            if [ "$(detect_platform)" = "macos" ] && [ ! -f "$shell_config" ]; then
                shell_config="$HOME/.bash_profile"
            fi
        else
            warning "Unsupported shell. Please manually add $bin_dir to your PATH"
            return
        fi

        if [ -f "$shell_config" ]; then
            if ! grep -q "export PATH.*$bin_dir" "$shell_config"; then
                echo "export PATH=\"$bin_dir:\$PATH\"" >> "$shell_config"
                info "Added to $shell_config. Run 'source $shell_config' to update your current session."
            fi
        else
            warning "Shell config file $shell_config not found. Please manually add $bin_dir to your PATH"
        fi
    fi
}

# Display usage
usage() {
    cat << EOF
HelixShell Installer

Usage: $0 [options]

Options:
  -h, --help           Show this help message
  -l, --local          Install to ~/.local/bin instead of /usr/local/bin
  -s, --system         Force system installation (requires sudo)
  -v, --version VER    Install specific version instead of latest

Environment variables:
  USE_SUDO=true        Force sudo for installation

Examples:
  curl -fsSL https://raw.githubusercontent.com/adarshanand67/Helix-Shell/main/install.sh | bash
  bash install.sh -l
  sudo bash install.sh -s

EOF
}

# Parse command line arguments
USE_SUDO=false
LOCAL_INSTALL=false
VERSION=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            exit 0
            ;;
        -l|--local)
            LOCAL_INSTALL=true
            shift
            ;;
        -s|--system)
            USE_SUDO=true
            LOCAL_INSTALL=false
            shift
            ;;
        -v|--version)
            VERSION="$2"
            shift 2
            ;;
        *)
            error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Check if running with sudo
if [ "$EUID" -eq 0 ]; then
    warning "Running as root is not recommended. Use -l for user installation."
fi

# Get platform and architecture
PLATFORM=$(detect_platform)
ARCH=$(detect_arch)
info "Detected platform: $PLATFORM-$ARCH"

# Determine installation path
if [ "$LOCAL_INSTALL" = "true" ] || [ "$USE_SUDO" != "true" ] && [ ! -w "/usr/local/bin" ]; then
    BIN_DIR="$HOME/.local/bin"
    BIN_PATH="$BIN_DIR/helix"
    mkdir -p "$BIN_DIR"
else
    BIN_DIR="/usr/local/bin"
    BIN_PATH="$BIN_DIR/helix"
    USE_SUDO=true
fi

# Get release information
info "Fetching latest release information..."
RELEASE_INFO=$(get_latest_release)

if [ -n "$VERSION" ]; then
    info "Using version: $VERSION"
else
    VERSION=$(echo "$RELEASE_INFO" | jq -r '.tag_name // empty' 2>/dev/null || echo "")
    if [ -z "$VERSION" ]; then
        VERSION="latest"
    fi
    info "Latest version: $VERSION"
fi

# Find binary URL
BINARY_URL=$(find_binary_url "$RELEASE_INFO" "$PLATFORM" "$ARCH")

# Download binary
TEMP_DIR=$(mktemp -d)
BINARY_PATH="$TEMP_DIR/helix"
download "$BINARY_URL" "$BINARY_PATH"

# Verify binary
if [ ! -f "$BINARY_PATH" ]; then
    error "Download failed"
    exit 1
fi

# Install binary
install_binary "$BINARY_PATH" "$BIN_PATH"

# Setup PATH for local installation
if [ "$LOCAL_INSTALL" = "true" ] || [ "$BIN_DIR" = "$HOME/.local/bin" ]; then
    setup_path
fi

# Cleanup
rm -rf "$TEMP_DIR"

# Verify installation
if command -v helix >/dev/null 2>&1; then
    success "HelixShell installed successfully!"
    info "Run 'helix --help' to get started"
else
    warning "Installation completed, but 'helix' is not in PATH"
    info "Add $BIN_DIR to your PATH or run $BIN_PATH directly"
fi

success "Installation complete! 🎉"
echo ""
echo "HelixShell features:"
echo "• Advanced autocompletion (TAB)"
echo "• Colored prompt with Git integration"
echo "• Command history (↑/↓ arrows)"
echo "• Job control (jobs, fg, bg)"
echo "• Pipeline support (|)"
echo "• I/O redirection (>, <, >>)"
echo ""
echo "For more information, visit: https://github.com/adarshanand67/Helix-Shell"
