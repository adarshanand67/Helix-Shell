# Multi-stage build for HelixShell
# Stage 1: Build environment
FROM ubuntu:22.04 AS builder

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    pkg-config \
    libcppunit-dev \
    libreadline-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source files
COPY src/ ./src/
COPY include/ ./include/
COPY tests/ ./tests/
COPY Makefile ./

# Build the shell
RUN make build

# Run tests to ensure build is valid
RUN make test

# Stage 2: Runtime environment
FROM ubuntu:22.04

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libreadline8 \
    git \
    curl \
    wget \
    vim \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user for running the shell
RUN useradd -m -s /bin/bash shelluser && \
    echo "shelluser:shelluser" | chpasswd

# Copy built binary from builder stage
COPY --from=builder /build/build/hsh /usr/local/bin/helix

# Make it executable
RUN chmod +x /usr/local/bin/helix

# Set up working directory
WORKDIR /home/shelluser

# Copy example files and documentation
COPY README.md /usr/local/share/doc/helix/
COPY --chown=shelluser:shelluser . /home/shelluser/helix-source/

# Switch to non-root user
USER shelluser

# Set environment variables
ENV SHELL=/usr/local/bin/helix
ENV TERM=xterm-256color
ENV HOME=/home/shelluser

# Create a welcome script
RUN echo '#!/bin/bash\n\
echo ""\n\
echo "🚀 Welcome to HelixShell Docker Container!"\n\
echo ""\n\
echo "HelixShell v1.0.0 - A modern Unix shell"\n\
echo ""\n\
echo "Features:"\n\
echo "  • TAB autocompletion"\n\
echo "  • Colored prompt with Git integration"\n\
echo "  • Command history (↑/↓)"\n\
echo "  • Job control (jobs, fg, bg)"\n\
echo "  • Pipelines and I/O redirection"\n\
echo ""\n\
echo "Type '\''exit'\'' to quit the shell"\n\
echo "Type '\''help'\'' for available commands"\n\
echo ""\n\
exec /usr/local/bin/helix\n\
' > /home/shelluser/start-helix.sh && chmod +x /home/shelluser/start-helix.sh

# Default command: start HelixShell
CMD ["/home/shelluser/start-helix.sh"]
