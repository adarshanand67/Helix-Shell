# Homebrew Tap for HelixShell

This repository contains the Homebrew formula for installing HelixShell.

## Installation

### Option 1: Install from this tap (recommended for development)

```bash
# Add the tap
brew tap adarshanand67/helix-shell https://github.com/adarshanand67/homebrew-helix-shell

# Install HelixShell
brew install helix-shell
```

### Option 2: Install directly (without adding tap)

```bash
brew install adarshanand67/helix-shell/helix-shell
```

### Option 3: Install from HEAD (latest development version)

```bash
brew install --HEAD helix-shell
```

## Usage

After installation, run the shell with:

```bash
helix
```

To use HelixShell as your default shell:

```bash
# Add to available shells
echo "$(brew --prefix)/bin/helix" | sudo tee -a /etc/shells

# Change default shell
chsh -s $(brew --prefix)/bin/helix
```

## Features

- **Advanced Autocompletion**: TAB completion for commands and file paths
- **Colored Prompt**: Beautiful prompt with Git branch integration
- **Command History**: Navigate with arrow keys, search with Ctrl+R
- **Job Control**: Background jobs with `jobs`, `fg`, `bg` commands
- **Pipelines**: Chain commands with `|`
- **I/O Redirection**: Support for `>`, `<`, `>>`, `2>`, `&>`

## Uninstallation

```bash
brew uninstall helix-shell
brew untap adarshanand67/helix-shell  # Optional: remove the tap
```

## Development

### Testing the formula locally

```bash
# Audit the formula
brew audit --strict --online helix-shell

# Test installation
brew install --build-from-source helix-shell

# Run tests
brew test helix-shell
```

### Updating the formula

When a new version is released:

1. Update the `url` with the new version tag
2. Calculate new SHA256 hash:
   ```bash
   curl -L https://github.com/adarshanand67/Helix-Shell/archive/refs/tags/vX.Y.Z.tar.gz | shasum -a 256
   ```
3. Update the `sha256` field in the formula
4. Commit and push changes

## Support

- **Homepage**: https://github.com/adarshanand67/Helix-Shell
- **Issues**: https://github.com/adarshanand67/Helix-Shell/issues
- **Documentation**: https://github.com/adarshanand67/Helix-Shell/blob/master/README.md
