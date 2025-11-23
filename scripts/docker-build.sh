#!/bin/bash

# Docker build script for HelixShell
# Builds the Docker image with proper tags

set -e

VERSION=${1:-"latest"}
IMAGE_NAME="helixshell"
DOCKER_USER="adarshanand67"

echo "🐳 Building HelixShell Docker image..."
echo "Version: $VERSION"
echo ""

# Build the image
docker build \
  --tag "${IMAGE_NAME}:${VERSION}" \
  --tag "${IMAGE_NAME}:latest" \
  .

echo ""
echo "✅ Build complete!"
echo ""
echo "Images created:"
docker images | grep helixshell | head -2
echo ""
echo "To run:"
echo "  docker run -it --rm ${IMAGE_NAME}:${VERSION}"
echo ""
echo "To tag for Docker Hub:"
echo "  docker tag ${IMAGE_NAME}:${VERSION} ${DOCKER_USER}/${IMAGE_NAME}:${VERSION}"
echo ""
echo "To push to Docker Hub:"
echo "  docker push ${DOCKER_USER}/${IMAGE_NAME}:${VERSION}"
echo "  docker push ${DOCKER_USER}/${IMAGE_NAME}:latest"
