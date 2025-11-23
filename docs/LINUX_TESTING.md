# Linux Testing Guide for HelixShell

This guide provides instructions for building, testing, and distributing HelixShell on Linux distributions, with a focus on Debian/Ubuntu systems.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Building from Source](#building-from-source)
3. [Running Tests](#running-tests)
4. [Installation Methods](#installation-methods)
5. [Creating a Debian Package](#creating-a-debian-package)
6. [Setting up Ubuntu PPA](#setting-up-ubuntu-ppa)
7. [Docker Testing](#docker-testing)
8. [Continuous Integration](#continuous-integration)

## Prerequisites

### Debian/Ubuntu Systems

Install required build dependencies:

```bash
# Update package list
sudo apt update

# Install build essentials
sudo apt install -y \
    build-essential \
    cmake \
    pkg-config \
    libcppunit-dev \
    libreadline-dev \
    git

# Verify installations
gcc --version        # Should be GCC 9+ for C++23 support
cmake --version      # Should be CMake 3.15+
pkg-config --version
```

### Other Linux Distributions

**Fedora/RHEL/CentOS:**
```bash
sudo dnf install -y gcc-c++ cmake pkg-config cppunit-devel readline-devel git
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake pkg-config cppunit readline git
```

**Alpine Linux:**
```bash
sudo apk add build-base cmake pkgconfig cppunit-dev readline-dev git
```

## Building from Source

### Standard Build

```bash
# Clone the repository
git clone https://github.com/adarshanand67/Helix-Shell.git
cd Helix-Shell

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build the project
make -j$(nproc)

# The executable will be at: build/helix
```

### Build with Debug Symbols

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

### Build with Address Sanitizer (for detecting memory issues)

```bash
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address -g" ..
make -j$(nproc)
```

### Build with Thread Sanitizer (for detecting race conditions)

```bash
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" ..
make -j$(nproc)
```

## Running Tests

### Run All Tests

```bash
cd build
./hsh_tests

# Or using make
make test
```

### Run with Valgrind (Memory Leak Detection)

```bash
# Install valgrind
sudo apt install valgrind

# Run tests with valgrind
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./hsh_tests
```

### Run with GDB (Debugging)

```bash
# Build with debug symbols first
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Run with GDB
gdb ./hsh_tests
# In GDB:
# (gdb) run
# (gdb) backtrace  (if crash occurs)
```

### Performance Profiling

```bash
# Install perf tools
sudo apt install linux-tools-common linux-tools-generic

# Profile execution
perf record ./hsh_tests
perf report
```

## Installation Methods

### Method 1: Local Installation (make install)

```bash
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install

# This installs:
# - /usr/local/bin/helix (executable)
# - /usr/local/share/man/man1/helix.1 (man page, if exists)
```

### Method 2: Using checkinstall (Debian package creation)

```bash
# Install checkinstall
sudo apt install checkinstall

cd build
cmake ..
make

# Create and install .deb package
sudo checkinstall --pkgname=helix-shell \
                  --pkgversion="2.0" \
                  --pkgrelease="1" \
                  --pkglicense="MIT" \
                  --maintainer="your-email@example.com" \
                  --requires="libreadline8,libcppunit-1.15-0" \
                  make install

# This creates a .deb file and installs it
# Later you can uninstall with: sudo apt remove helix-shell
```

### Method 3: Manual Installation

```bash
cd build
make

# Copy executable
sudo cp helix /usr/local/bin/

# Make it executable
sudo chmod +x /usr/local/bin/helix

# Test installation
helix --version
```

## Creating a Debian Package

### Step 1: Create Debian Package Structure

```bash
cd HelixShell
mkdir -p debian

# Create debian/control file
cat > debian/control << 'EOF'
Source: helix-shell
Section: shells
Priority: optional
Maintainer: Your Name <your-email@example.com>
Build-Depends: debhelper-compat (= 13), cmake, pkg-config, libcppunit-dev, libreadline-dev
Standards-Version: 4.6.0
Homepage: https://github.com/adarshanand67/Helix-Shell

Package: helix-shell
Architecture: any
Depends: ${shlibs:Depends}, ${misc:Depends}, libreadline8
Description: Modern Unix shell with advanced features
 HelixShell is a modern Unix shell with TAB autocompletion,
 colored prompts with Git integration, job control, pipelines,
 and I/O redirection.
 .
 Features include:
  - Command autocompletion
  - Git-aware colored prompt
  - Full pipeline support
  - Advanced I/O redirection
  - Job control (background/foreground)
EOF

# Create debian/rules file
cat > debian/rules << 'EOF'
#!/usr/bin/make -f

%:
	dh $@ --buildsystem=cmake

override_dh_auto_configure:
	dh_auto_configure -- -DCMAKE_BUILD_TYPE=Release

override_dh_auto_test:
	dh_auto_test || true
EOF

chmod +x debian/rules

# Create debian/changelog
cat > debian/changelog << 'EOF'
helix-shell (2.0-1) unstable; urgency=medium

  * Initial release with composition-based architecture
  * Implemented Dependency Inversion Principle
  * Added comprehensive interface-based design
  * Full SOLID principles compliance

 -- Your Name <your-email@example.com>  $(date -R)
EOF

# Create debian/compat
echo "13" > debian/compat

# Create debian/source/format
mkdir -p debian/source
echo "3.0 (native)" > debian/source/format
```

### Step 2: Build the Package

```bash
# Install build tools
sudo apt install debhelper devscripts

# Build the package
dpkg-buildpackage -us -uc -b

# Package will be created in parent directory:
# ../helix-shell_2.0-1_amd64.deb
```

### Step 3: Install the Package

```bash
sudo dpkg -i ../helix-shell_2.0-1_amd64.deb

# Fix dependencies if needed
sudo apt --fix-broken install

# Test installation
helix --version
which helix
```

### Step 4: Verify Package Contents

```bash
# List package contents
dpkg -L helix-shell

# Show package info
dpkg -s helix-shell

# Remove package
sudo apt remove helix-shell
```

## Setting up Ubuntu PPA

### Prerequisites

1. **Launchpad Account**: Create account at https://launchpad.net
2. **GPG Key**: For signing packages

```bash
# Generate GPG key
gpg --full-generate-key
# Choose: RSA and RSA, 4096 bits, no expiration
# Enter your name and email (must match Launchpad email)

# List your keys
gpg --list-keys

# Upload to Ubuntu keyserver
gpg --keyserver keyserver.ubuntu.com --send-keys YOUR_KEY_ID
```

### Create PPA

1. Go to https://launchpad.net/~your-username
2. Click "Create a new PPA"
3. Name: `helix-shell`
4. Description: "Modern Unix shell with advanced features"
5. Click "Activate"

### Build Source Package

```bash
# Create source package structure
cd HelixShell

# Update debian/changelog with PPA target
cat > debian/changelog << 'EOF'
helix-shell (2.0-1ubuntu1~jammy1) jammy; urgency=medium

  * Initial PPA release for Ubuntu 22.04 (Jammy)
  * Composition-based architecture
  * Full SOLID principles compliance

 -- Your Name <your-email@example.com>  $(date -R)
EOF

# Build source package
debuild -S -sa

# Sign with your GPG key (will prompt for passphrase)
```

### Upload to PPA

```bash
# Install dput
sudo apt install dput

# Create ~/.dput.cf if it doesn't exist
cat > ~/.dput.cf << 'EOF'
[helix-shell-ppa]
fqdn = ppa.launchpad.net
method = ftp
incoming = ~your-username/ubuntu/helix-shell/
login = anonymous
allow_unsigned_uploads = 0
EOF

# Upload to PPA
dput helix-shell-ppa ../helix-shell_2.0-1ubuntu1~jammy1_source.changes

# Wait for Launchpad to build (check https://launchpad.net/~your-username/+archive/ubuntu/helix-shell)
```

### User Installation from PPA

Once published, users can install with:

```bash
sudo add-apt-repository ppa:your-username/helix-shell
sudo apt update
sudo apt install helix-shell
```

### Multi-Release Support

For supporting multiple Ubuntu versions:

```bash
# Build for Ubuntu 22.04 (Jammy)
dch -v 2.0-1ubuntu1~jammy1 -D jammy "Build for Jammy"
debuild -S -sa
dput helix-shell-ppa ../helix-shell_2.0-1ubuntu1~jammy1_source.changes

# Build for Ubuntu 23.04 (Lunar)
dch -v 2.0-1ubuntu1~lunar1 -D lunar "Build for Lunar"
debuild -S -sa
dput helix-shell-ppa ../helix-shell_2.0-1ubuntu1~lunar1_source.changes

# Build for Ubuntu 23.10 (Mantic)
dch -v 2.0-1ubuntu1~mantic1 -D mantic "Build for Mantic"
debuild -S -sa
dput helix-shell-ppa ../helix-shell_2.0-1ubuntu1~mantic1_source.changes
```

## Docker Testing

### Build and Test in Debian Container

```bash
# Create Dockerfile for testing
cat > Dockerfile.debian << 'EOF'
FROM debian:bookworm

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libcppunit-dev \
    libreadline-dev \
    git

WORKDIR /build
COPY . .

RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc) && \
    ./hsh_tests

CMD ["/build/build/helix"]
EOF

# Build and run
docker build -f Dockerfile.debian -t helix-debian .
docker run -it helix-debian
```

### Build and Test in Ubuntu Container

```bash
cat > Dockerfile.ubuntu << 'EOF'
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libcppunit-dev \
    libreadline-dev \
    git

WORKDIR /build
COPY . .

RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc) && \
    ./hsh_tests

CMD ["/build/build/helix"]
EOF

docker build -f Dockerfile.ubuntu -t helix-ubuntu .
docker run -it helix-ubuntu
```

### Test Across Multiple Distributions

```bash
# Test on different distros
for distro in ubuntu:22.04 debian:bookworm fedora:38 archlinux:latest; do
    echo "Testing on $distro..."
    docker run --rm -v $(pwd):/src $distro bash -c "
        cd /src &&
        # Install deps (distro-specific) &&
        mkdir -p build && cd build &&
        cmake .. && make && ./hsh_tests
    "
done
```

## Continuous Integration

### GitHub Actions for Linux Testing

Create `.github/workflows/linux-ci.yml`:

```yaml
name: Linux CI

on: [push, pull_request]

jobs:
  test-ubuntu:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        ubuntu-version: ['20.04', '22.04', '24.04']

    steps:
    - uses: actions/checkout@v3

    - name: Install dependencies
      run: |
        sudo apt update
        sudo apt install -y build-essential cmake pkg-config libcppunit-dev libreadline-dev

    - name: Build
      run: |
        mkdir build && cd build
        cmake ..
        make -j$(nproc)

    - name: Run tests
      run: |
        cd build
        ./hsh_tests

    - name: Run with Valgrind
      run: |
        sudo apt install -y valgrind
        cd build
        valgrind --leak-check=full --error-exitcode=1 ./hsh_tests

  test-debian:
    runs-on: ubuntu-latest
    container: debian:bookworm

    steps:
    - uses: actions/checkout@v3

    - name: Install dependencies
      run: |
        apt update
        apt install -y build-essential cmake pkg-config libcppunit-dev libreadline-dev

    - name: Build and test
      run: |
        mkdir build && cd build
        cmake ..
        make -j$(nproc)
        ./hsh_tests
```

## Troubleshooting

### Issue: "command not found: helix"

**Solution:**
```bash
# Check if installed
which helix

# Add to PATH if needed
echo 'export PATH="/usr/local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

### Issue: Build fails with C++23 errors

**Solution:**
```bash
# Check GCC version (need GCC 11+)
gcc --version

# Upgrade if needed (Ubuntu 22.04+)
sudo apt install gcc-11 g++-11
export CC=gcc-11
export CXX=g++-11
```

### Issue: Missing libreadline

**Solution:**
```bash
sudo apt install libreadline-dev libreadline8
```

### Issue: CMake version too old

**Solution:**
```bash
# Remove old cmake
sudo apt remove cmake

# Install from Kitware repository
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt update
sudo apt install cmake
```

## Distribution-Specific Notes

### Ubuntu 20.04 (Focal)
- Requires GCC 11+ from PPA
- May need newer CMake from Kitware

### Ubuntu 22.04 (Jammy)
- Works out of the box with default packages
- Recommended for development

### Debian 11 (Bullseye)
- Requires GCC 11+ from backports
- Install: `sudo apt install -t bullseye-backports g++-11`

### Debian 12 (Bookworm)
- Full support with default packages
- Recommended for production

## Performance Benchmarks

Run performance tests on Linux:

```bash
# Build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# Benchmark command execution
time ./helix -c "ls /usr/bin | grep -E '^a' | wc -l"

# Benchmark pipeline performance
time ./helix -c "cat large_file.txt | grep pattern | sort | uniq > output.txt"

# Compare with bash
time bash -c "cat large_file.txt | grep pattern | sort | uniq > output.txt"
```

## Resources

- **Debian Packaging Guide**: https://www.debian.org/doc/manuals/maint-guide/
- **Ubuntu Packaging Guide**: https://packaging.ubuntu.com/html/
- **Launchpad PPA Help**: https://help.launchpad.net/Packaging/PPA
- **CMake Documentation**: https://cmake.org/documentation/
- **CppUnit Documentation**: https://freedesktop.org/wiki/Software/cppunit/

## Support

For Linux-specific issues:
- File bug reports: https://github.com/adarshanand67/Helix-Shell/issues
- Tag with `linux`, `debian`, or `ubuntu` labels
- Include output of `uname -a` and `lsb_release -a`
