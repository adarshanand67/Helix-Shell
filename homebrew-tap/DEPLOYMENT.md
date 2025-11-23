# Homebrew Tap Deployment Guide

This guide walks you through setting up a Homebrew tap for HelixShell so users can install it with `brew install helix-shell`.

## Step 1: Create a GitHub Release

First, create a versioned release of HelixShell:

```bash
# Make sure everything is committed
cd /Users/adarsh_anand/Documents/HelixShell
git status

# Create a version tag
git tag -a v1.0.0 -m "Release version 1.0.0 - Initial stable release"

# Push the tag to GitHub
git push origin v1.0.0
```

Then on GitHub:
1. Go to https://github.com/adarshanand67/Helix-Shell/releases
2. Click "Draft a new release"
3. Select the tag `v1.0.0`
4. Fill in release notes (features, improvements, etc.)
5. Click "Publish release"

GitHub will automatically create a source tarball at:
```
https://github.com/adarshanand67/Helix-Shell/archive/refs/tags/v1.0.0.tar.gz
```

## Step 2: Calculate SHA256 Hash

Download the release tarball and calculate its SHA256:

```bash
# Download and calculate hash in one command
curl -L https://github.com/adarshanand67/Helix-Shell/archive/refs/tags/v1.0.0.tar.gz | shasum -a 256
```

Copy the output hash (64 character hexadecimal string).

## Step 3: Update the Formula

Edit `homebrew-tap/Formula/helix-shell.rb`:

```ruby
url "https://github.com/adarshanand67/Helix-Shell/archive/refs/tags/v1.0.0.tar.gz"
sha256 "PASTE_THE_HASH_HERE"
```

## Step 4: Create the Homebrew Tap Repository

Create a **new** GitHub repository named `homebrew-helix-shell`:

```bash
# Navigate to a separate directory (not inside HelixShell)
cd ~/Documents
mkdir homebrew-helix-shell
cd homebrew-helix-shell

# Initialize the repository
git init
git branch -M main

# Copy the formula files
cp -r /Users/adarsh_anand/Documents/HelixShell/homebrew-tap/* .

# Create initial commit
git add .
git commit -m "Add HelixShell formula v1.0.0"

# Create the repository on GitHub, then:
git remote add origin https://github.com/adarshanand67/homebrew-helix-shell.git
git push -u origin main
```

**Important**: The repository **must** be named `homebrew-<name>` for Homebrew to recognize it as a tap.

## Step 5: Test the Formula

Test that users can install from your tap:

```bash
# Add your tap
brew tap adarshanand67/helix-shell

# Install HelixShell
brew install helix-shell

# Test it works
helix --version  # or just run: helix

# Test autocompletion
# Type 'ls' and press TAB

# Uninstall (for testing)
brew uninstall helix-shell
```

## Step 6: Announce to Users

Update the main HelixShell README.md with installation instructions:

```markdown
## Installation

### macOS (Homebrew)

\`\`\`bash
brew tap adarshanand67/helix-shell
brew install helix-shell
\`\`\`

### Linux

\`\`\`bash
git clone https://github.com/adarshanand67/Helix-Shell.git
cd Helix-Shell
./setup.sh
make build
sudo make install  # If you create an install target
\`\`\`
```

## Step 7: Future Updates

When you release a new version:

1. **Create new tag and release**:
   ```bash
   git tag -a v1.1.0 -m "Release v1.1.0 - New features"
   git push origin v1.1.0
   ```

2. **Update the formula**:
   ```bash
   cd ~/Documents/homebrew-helix-shell

   # Calculate new hash
   curl -L https://github.com/adarshanand67/Helix-Shell/archive/refs/tags/v1.1.0.tar.gz | shasum -a 256

   # Edit Formula/helix-shell.rb
   # - Update url to v1.1.0
   # - Update sha256 with new hash

   git add Formula/helix-shell.rb
   git commit -m "Update HelixShell to v1.1.0"
   git push
   ```

3. **Users update**:
   ```bash
   brew update
   brew upgrade helix-shell
   ```

## Alternative: Submit to Homebrew Core

To get HelixShell into the official Homebrew repository (so users can just `brew install helix-shell` without adding a tap):

1. Your project needs to be stable and well-maintained
2. Have significant user base/stars
3. Follow Homebrew's guidelines strictly
4. Submit a PR to https://github.com/Homebrew/homebrew-core

This is a more advanced step and recommended after your project gains traction.

## Repository Structure

Your `homebrew-helix-shell` repository should look like:

```
homebrew-helix-shell/
├── Formula/
│   └── helix-shell.rb
├── README.md
└── DEPLOYMENT.md (this file)
```

## Troubleshooting

### Formula audit fails

```bash
brew audit --strict helix-shell
# Fix any issues it reports
```

### Installation fails

```bash
brew install --verbose --debug helix-shell
# Check the output for errors
```

### Users can't find the tap

Make sure:
- Repository is public
- Repository name is `homebrew-<name>`
- Formula file is in `Formula/` directory
- Formula class name matches filename (HelixShell ↔ helix-shell.rb)
