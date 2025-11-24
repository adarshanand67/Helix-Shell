# Release Process

This document describes how to create a new release of Helix Shell.

## Prerequisites

- **GitHub CLI (`gh`)**: Install with `brew install gh` (macOS) or see [installation guide](https://cli.github.com/)
- **GitHub Authentication**: Run `gh auth login` if not already authenticated
- **CMake and Build Tools**: Ensure you can build the project locally
- **Write Access**: You need push access to the repository

## Release Methods

### Method 1: Automated GitHub Actions (Recommended)

This method builds binaries for all platforms (Linux, macOS, Windows) automatically.

1. **Tag and push the version:**
   ```bash
   git tag v1.2.0
   git push origin v1.2.0
   ```

2. **GitHub Actions will automatically:**
   - Build binaries for Linux (x86_64)
   - Build universal binary for macOS (ARM64 + Intel)
   - Build binary for Windows (x86_64)
   - Create GitHub release with all binaries
   - Generate release notes from commits

3. **Monitor the release:**
   - Go to https://github.com/adarshanand67/Helix-Shell/actions
   - Watch the "Release" workflow
   - Release will be published at https://github.com/adarshanand67/Helix-Shell/releases

### Method 2: Local Release Script (macOS only)

Use this for quick macOS-only releases or testing.

1. **Run the release script:**
   ```bash
   ./scripts/create-release.sh v1.2.0
   ```

2. **The script will:**
   - Validate version format
   - Build ARM64, Intel, and Universal binaries for macOS
   - Create `.tar.gz` archives
   - Generate release notes
   - Create GitHub release with `gh` CLI
   - Upload macOS binaries

3. **Manual additions:**
   - Linux and Windows binaries need to be built separately and uploaded

## Version Numbering

Follow [Semantic Versioning](https://semver.org/):

- **Major** (v**2**.0.0): Breaking changes, major new features
- **Minor** (v1.**2**.0): New features, backwards-compatible
- **Patch** (v1.0.**1**): Bug fixes, minor improvements

Examples:
- `v1.0.0` - Initial stable release
- `v1.1.0` - Added new job control features
- `v1.1.1` - Fixed bug in signal handling
- `v2.0.0` - Complete rewrite with breaking changes

## Release Checklist

Before creating a release:

- [ ] All tests pass (`./build/hsh_tests`)
- [ ] CI/CD pipeline is green
- [ ] Documentation is updated
- [ ] CHANGELOG.md is updated (if you maintain one)
- [ ] Version number follows semantic versioning
- [ ] No uncommitted changes
- [ ] On the correct branch (usually `master`)

After release:

- [ ] Verify release on GitHub
- [ ] Test download and installation
- [ ] Update Homebrew formula (if applicable)
- [ ] Update Docker image tags
- [ ] Announce on relevant channels
- [ ] Update project website/docs

## Post-Release Updates

### Update Homebrew Formula

If you maintain a Homebrew tap:

```bash
cd /path/to/homebrew-helix-shell
# Update version and SHA256 in Formula/helix-shell.rb
brew audit --strict helix-shell
git commit -am "Update to version v1.2.0"
git push
```

### Update Docker Image

Tag and push Docker image with the new version:

```bash
docker build -t adarshanand67/helixshell:v1.2.0 .
docker tag adarshanand67/helixshell:v1.2.0 adarshanand67/helixshell:latest
docker push adarshanand67/helixshell:v1.2.0
docker push adarshanand67/helixshell:latest
```

## Troubleshooting

### GitHub Actions fails to build

1. Check the Actions tab for error logs
2. Test the build locally first
3. Ensure all dependencies are specified in the workflow
4. Check platform-specific issues (Windows, Linux, macOS)

### `gh` CLI authentication issues

```bash
gh auth login
# Follow the prompts to authenticate
```

### Build failures

```bash
# Clean build
rm -rf build
mkdir build && cd build
cmake ..
make

# Run tests
./hsh_tests
```

### Release already exists

If you need to recreate a release:

```bash
# Delete the release and tag
gh release delete v1.2.0 --yes
git tag -d v1.2.0
git push origin :refs/tags/v1.2.0

# Then create the release again
./scripts/create-release.sh v1.2.0
```

## Example Release Process

Here's a complete example of releasing version v1.2.0:

```bash
# 1. Ensure you're on master and up to date
git checkout master
git pull origin master

# 2. Run tests
rm -rf build && mkdir build && cd build
cmake .. && make
./hsh_tests
cd ..

# 3. Create and push tag
git tag v1.2.0
git push origin v1.2.0

# 4. GitHub Actions will create the release automatically
# Monitor at: https://github.com/adarshanand67/Helix-Shell/actions

# 5. Verify the release
open https://github.com/adarshanand67/Helix-Shell/releases/tag/v1.2.0

# 6. Test the release
curl -LO https://github.com/adarshanand67/Helix-Shell/releases/download/v1.2.0/helix-v1.2.0-macos-universal.tar.gz
tar xzf helix-v1.2.0-macos-universal.tar.gz
./helix-macos-universal --version
```

## Release Notes Template

When manually creating release notes, use this template:

```markdown
# Helix Shell vX.Y.Z

## What's New

- Feature 1: Brief description
- Feature 2: Brief description
- Bug fix: Brief description

## Breaking Changes

- List any breaking changes here
- Upgrade instructions if needed

## Installation

[Standard installation instructions]

## Full Changelog

**Full Changelog**: https://github.com/adarshanand67/Helix-Shell/compare/vX.Y-1.Z...vX.Y.Z
```

## Support

For questions or issues with the release process:
- Open an issue: https://github.com/adarshanand67/Helix-Shell/issues
- Contact: [your contact info]
