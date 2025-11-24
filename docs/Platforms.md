Native package managers
• Linux: apt (Debian or Ubuntu PPA), dnf/rpm (Fedora, RHEL, CentOS), pacman (Arch AUR), zypper (openSUSE)
• Windows: Winget, Chocolatey, Scoop
• BSD: FreeBSD Ports, OpenBSD Ports, NetBSD pkgsrc

Cross-platform installers
• Snapcraft (Snap packages)
• Flatpak
• AppImage
• Nixpkgs (NixOS and nix-enabled systems)
• Conda-forge (if you want a Conda package)

Container and cloud registries
• GitHub Container Registry
• GitLab Container Registry
• Amazon ECR Public
• Google Artifact Registry
• Docker Hub (already done)
• OCI images via distribution-based registries (Harbor, Quay)

Developer ecosystems
• Homebrew (already done)
• vcpkg
• Conan package manager
• CPM (CMake package manager, via simple CMakeLists integration)
• pkgconfig metadata for Linux distros

Binary release channels
• GitHub Releases with prebuilt macOS, Linux, Windows binaries
• GitLab Releases
• SourceForge
• FossHub

OS-specific app stores (rare for CLI but possible)
• Microsoft Store via MSIX packaging
• macOS App Store (not common for CLI without UI)

Web-based distribution
• curl | bash installer script published on your website or GitHub
• Custom APT/YUM repository hosted on your domain
• Static binaries via CDN or cloud storage (AWS S3, Google Cloud Storage)