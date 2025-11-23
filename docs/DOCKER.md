# Docker Deployment Guide for HelixShell

This guide explains how to build, run, and deploy HelixShell using Docker.

## Table of Contents

- [Quick Start](#quick-start)
- [Building the Image](#building-the-image)
- [Running HelixShell](#running-helixshell)
- [Docker Compose](#docker-compose)
- [Publishing to Docker Hub](#publishing-to-docker-hub)
- [Advanced Usage](#advanced-usage)

## Quick Start

### Using Docker Run

```bash
# Build the image
docker build -t helixshell:latest .

# Run HelixShell
docker run -it --rm helixshell:latest
```

### Using Docker Compose

```bash
# Run HelixShell
docker-compose up helix

# Run in detached mode and attach
docker-compose up -d helix
docker attach helix-shell
```

## Building the Image

### Standard Build

```bash
docker build -t helixshell:latest .
```

### Build with Custom Tag

```bash
docker build -t helixshell:v1.0.0 .
```

### Multi-Platform Build (for distribution)

```bash
docker buildx build --platform linux/amd64,linux/arm64 -t helixshell:latest .
```

## Running HelixShell

### Interactive Mode (Recommended)

```bash
docker run -it --rm \
  --name helix \
  helixshell:latest
```

### With Persistent Workspace

```bash
# Create workspace directory
mkdir -p ./workspace

# Run with mounted workspace
docker run -it --rm \
  --name helix \
  -v $(pwd)/workspace:/home/shelluser/workspace \
  helixshell:latest
```

### With Git Configuration

```bash
docker run -it --rm \
  --name helix \
  -v ~/.gitconfig:/home/shelluser/.gitconfig:ro \
  -v $(pwd)/workspace:/home/shelluser/workspace \
  helixshell:latest
```

### Custom Entry Point

```bash
# Start with bash instead of helix
docker run -it --rm \
  --entrypoint /bin/bash \
  helixshell:latest
```

## Docker Compose

### Available Services

1. **helix** - Production shell environment
2. **helix-dev** - Development environment with source mounted
3. **helix-test** - Test runner

### Running Services

```bash
# Start main shell
docker-compose up helix

# Start development shell
docker-compose --profile dev up helix-dev

# Run tests
docker-compose --profile test up helix-test
```

### Background Mode

```bash
# Start in background
docker-compose up -d helix

# Attach to running shell
docker attach helix-shell

# Detach: Ctrl+P, Ctrl+Q

# Stop
docker-compose down
```

## Publishing to Docker Hub

### Prerequisites

```bash
# Login to Docker Hub
docker login
```

### Tag and Push

```bash
# Tag the image
docker tag helixshell:latest adarshanand67/helixshell:latest
docker tag helixshell:latest adarshanand67/helixshell:v1.0.0

# Push to Docker Hub
docker push adarshanand67/helixshell:latest
docker push adarshanand67/helixshell:v1.0.0
```

### Pull and Run from Docker Hub

Once published, users can run:

```bash
docker pull adarshanand67/helixshell:latest
docker run -it --rm adarshanand67/helixshell:latest
```

## Advanced Usage

### Multi-Stage Build Analysis

The Dockerfile uses multi-stage builds:

1. **Builder Stage**: Compiles HelixShell with all build dependencies
2. **Runtime Stage**: Minimal runtime environment with only necessary libraries

Benefits:
- Smaller final image (~200MB vs ~1GB)
- No build tools in production image
- Improved security (fewer attack surfaces)

### Image Size Comparison

```bash
# Check image sizes
docker images helixshell

# Expected sizes:
# Builder stage: ~1.2GB
# Final image: ~200MB
```

### Running Tests in Docker

```bash
# Build and run tests
docker build --target builder -t helixshell:test .
docker run --rm helixshell:test make test

# Or using docker-compose
docker-compose --profile test run --rm helix-test
```

### Development Workflow

```bash
# 1. Start dev container with source mounted
docker-compose --profile dev up -d helix-dev

# 2. Make changes to source code locally

# 3. Rebuild inside container
docker exec -it helix-shell-dev bash
cd /build
make clean && make build

# 4. Test changes
make test

# 5. Exit and restart shell
exit
docker-compose restart helix-dev
```

### Custom Dockerfile for Specific Needs

**Alpine-based (smaller image):**

```dockerfile
FROM alpine:latest
RUN apk add --no-cache g++ make readline-dev
# ... rest of build
```

**With additional tools:**

```dockerfile
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
    libreadline8 git curl wget vim tmux \
    python3 nodejs npm
# ... rest of build
```

## Environment Variables

- `SHELL=/usr/local/bin/helix` - Default shell
- `TERM=xterm-256color` - Terminal type for colors
- `HOME=/home/shelluser` - User home directory

## Volumes

### Recommended Mounts

```bash
docker run -it --rm \
  -v $(pwd)/workspace:/home/shelluser/workspace \
  -v ~/.gitconfig:/home/shelluser/.gitconfig:ro \
  -v ~/.ssh:/home/shelluser/.ssh:ro \
  helixshell:latest
```

## Security Considerations

- Container runs as non-root user (`shelluser`)
- Minimal runtime dependencies
- Read-only mounts for sensitive files (gitconfig, ssh)
- No privileged mode required

## Troubleshooting

### Colors Not Displaying

```bash
# Ensure TERM is set correctly
docker run -it --rm -e TERM=xterm-256color helixshell:latest
```

### Git Branch Not Showing

```bash
# Mount your .gitconfig
docker run -it --rm \
  -v ~/.gitconfig:/home/shelluser/.gitconfig:ro \
  -v $(pwd)/workspace:/home/shelluser/workspace \
  helixshell:latest
```

### Permission Issues

```bash
# Check user inside container
docker run --rm helixshell:latest id

# If needed, run as specific UID
docker run --rm --user $(id -u):$(id -g) helixshell:latest
```

### Build Failures

```bash
# Clean build without cache
docker build --no-cache -t helixshell:latest .

# Check build logs
docker build -t helixshell:latest . 2>&1 | tee build.log
```

## CI/CD Integration

### GitHub Actions Example

```yaml
name: Docker Build

on: [push]

jobs:
  docker:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Build Docker image
        run: docker build -t helixshell:latest .

      - name: Run tests in Docker
        run: docker run --rm helixshell:latest make test

      - name: Login to Docker Hub
        if: github.ref == 'refs/heads/master'
        run: echo "${{ secrets.DOCKER_PASSWORD }}" | docker login -u "${{ secrets.DOCKER_USERNAME }}" --password-stdin

      - name: Push to Docker Hub
        if: github.ref == 'refs/heads/master'
        run: |
          docker tag helixshell:latest adarshanand67/helixshell:latest
          docker push adarshanand67/helixshell:latest
```

## Performance Optimization

### Layer Caching

The Dockerfile is optimized for layer caching:
1. Dependencies installed first (rarely change)
2. Source code copied last (frequently changes)

### Build Cache

```bash
# Use BuildKit for better caching
DOCKER_BUILDKIT=1 docker build -t helixshell:latest .
```

## FAQ

**Q: Can I use HelixShell as my default shell in Docker?**
A: Yes, the image sets `SHELL=/usr/local/bin/helix` by default.

**Q: How do I persist command history?**
A: Mount a volume for the home directory:
```bash
docker run -it -v helix-home:/home/shelluser helixshell:latest
```

**Q: Can I run HelixShell on Windows/Mac?**
A: Yes, Docker Desktop supports Linux containers on all platforms.

**Q: What's the image size?**
A: ~200MB for the final runtime image (using multi-stage build).
