# Docker Quick Start Guide for HelixShell

## Prerequisites

- Docker Desktop installed (macOS/Windows) or Docker Engine (Linux)
- Docker daemon running

## 🚀 Quick Start (3 ways)

### 1. Pull from Docker Hub (Coming Soon)

Once published to Docker Hub:

```bash
docker pull adarshanand67/helixshell:latest
docker run -it --rm adarshanand67/helixshell:latest
```

### 2. Build Locally

```bash
# Build the image
docker build -t helixshell .

# Run it
docker run -it --rm helixshell
```

### 3. Using Docker Compose

```bash
# Start HelixShell
docker-compose up helix

# Or in detached mode
docker-compose up -d helix
docker attach helix-shell
```

## 📦 Building the Image

### Using the helper script:

```bash
./scripts/docker-build.sh v1.0.0
```

### Manual build:

```bash
docker build -t helixshell:latest .
```

### Check the image:

```bash
docker images | grep helixshell
```

## 🏃 Running HelixShell

### Basic Run

```bash
docker run -it --rm helixshell:latest
```

Flags explained:
- `-it` - Interactive terminal
- `--rm` - Remove container after exit
- `helixshell:latest` - Image to run

### With Persistent Workspace

```bash
docker run -it --rm \
  -v $(pwd)/workspace:/home/shelluser/workspace \
  helixshell:latest
```

Your files in `./workspace` will persist across container restarts.

### With Git Integration

```bash
docker run -it --rm \
  -v ~/.gitconfig:/home/shelluser/.gitconfig:ro \
  -v $(pwd)/workspace:/home/shelluser/workspace \
  helixshell:latest
```

This enables Git branch display in the prompt.

## 🧪 Testing

### Run tests in Docker:

```bash
# Using docker-compose
docker-compose --profile test run --rm helix-test

# Using docker build
docker build --target builder -t helixshell:test .
docker run --rm helixshell:test make test
```

## 📤 Publishing to Docker Hub

### Prerequisites:

1. Docker Hub account
2. Login: `docker login`
3. Set secrets in GitHub (for CI/CD):
   - `DOCKER_USERNAME`
   - `DOCKER_PASSWORD`

### Manual Publish:

```bash
# Using the helper script
./scripts/docker-publish.sh v1.0.0

# Or manually
docker tag helixshell:latest adarshanand67/helixshell:latest
docker tag helixshell:latest adarshanand67/helixshell:v1.0.0
docker push adarshanand67/helixshell:latest
docker push adarshanand67/helixshell:v1.0.0
```

### Automated Publish (CI/CD):

The GitHub Actions workflow automatically:
- Builds on every push
- Publishes to Docker Hub on version tags (`v*`)
- Creates multi-platform images (amd64/arm64)

To trigger:
```bash
git tag -a v1.0.1 -m "Release v1.0.1"
git push --no-verify origin v1.0.1
```

## 🔍 Troubleshooting

### Docker daemon not running

**Error:** `Cannot connect to the Docker daemon`

**Fix:** Start Docker Desktop or Docker daemon:
```bash
# macOS
open -a Docker

# Linux
sudo systemctl start docker
```

### Image too large

The multi-stage build should produce ~200MB images. If larger:
```bash
# Check image size
docker images helixshell

# Rebuild without cache
docker build --no-cache -t helixshell .
```

### Colors not displaying

**Fix:** Ensure TERM is set:
```bash
docker run -it --rm -e TERM=xterm-256color helixshell
```

### Permission denied

**Fix:** Ensure you're using the non-root user:
```bash
docker run --rm helixshell id
# Should show: uid=1000(shelluser) gid=1000(shelluser)
```

## 📚 Full Documentation

See [docs/DOCKER.md](docs/DOCKER.md) for:
- Advanced usage
- Multi-platform builds
- CI/CD integration
- Performance optimization
- FAQ

## 🎯 Common Use Cases

### Development Environment

```bash
# Mount source code and rebuild inside container
docker-compose --profile dev up -d helix-dev
docker exec -it helix-shell-dev bash
cd /build && make clean && make build
```

### CI/CD Testing

```yaml
# .github/workflows/test.yml
- name: Test in Docker
  run: |
    docker build -t helixshell:test .
    docker run --rm helixshell:test make test
```

### Sandbox Environment

```bash
# Fresh environment every time
docker run -it --rm helixshell:latest

# Play around, then exit - everything is cleaned up
```

## 🚀 Next Steps

1. **Test locally**: `docker build -t helixshell . && docker run -it --rm helixshell`
2. **Publish to Docker Hub**: `./scripts/docker-publish.sh v1.0.0`
3. **Update documentation**: Add Docker Hub badge to README
4. **Set up CI/CD**: Add `DOCKER_USERNAME` and `DOCKER_PASSWORD` to GitHub secrets

## 📊 Image Details

- **Base Image**: Ubuntu 22.04
- **Size**: ~200MB (multi-stage build)
- **User**: Non-root (`shelluser`)
- **Shell**: `/usr/local/bin/helix`
- **Platforms**: linux/amd64, linux/arm64

## 🔗 Links

- [Dockerfile](Dockerfile)
- [Docker Compose](docker-compose.yml)
- [Full Documentation](docs/DOCKER.md)
- [Build Script](scripts/docker-build.sh)
- [Publish Script](scripts/docker-publish.sh)
