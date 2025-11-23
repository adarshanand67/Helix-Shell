#!/bin/bash

# Docker publish script for HelixShell
# Tags and pushes images to Docker Hub

set -e

VERSION=${1:-"latest"}
IMAGE_NAME="helixshell"
DOCKER_USER="adarshanand67"

echo "🐳 Publishing HelixShell to Docker Hub..."
echo "Version: $VERSION"
echo "Repository: ${DOCKER_USER}/${IMAGE_NAME}"
echo ""

# Check if logged in
if ! docker info | grep -q "Username"; then
    echo "⚠️  Not logged in to Docker Hub"
    echo "Please run: docker login"
    exit 1
fi

# Tag images
echo "🏷️  Tagging images..."
docker tag ${IMAGE_NAME}:${VERSION} ${DOCKER_USER}/${IMAGE_NAME}:${VERSION}
docker tag ${IMAGE_NAME}:latest ${DOCKER_USER}/${IMAGE_NAME}:latest

# Push images
echo "📤 Pushing to Docker Hub..."
docker push ${DOCKER_USER}/${IMAGE_NAME}:${VERSION}
docker push ${DOCKER_USER}/${IMAGE_NAME}:latest

echo ""
echo "✅ Published successfully!"
echo ""
echo "Users can now pull with:"
echo "  docker pull ${DOCKER_USER}/${IMAGE_NAME}:${VERSION}"
echo "  docker pull ${DOCKER_USER}/${IMAGE_NAME}:latest"
echo ""
echo "Run with:"
echo "  docker run -it --rm ${DOCKER_USER}/${IMAGE_NAME}:${VERSION}"
