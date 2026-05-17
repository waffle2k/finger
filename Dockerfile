# Multi-stage build for C++ finger service
# Build stage - use Ubuntu with all development dependencies
FROM ubuntu:24.04 AS builder

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    meson \
    ninja-build \
    pkg-config \
    libboost-all-dev \
    libgtest-dev \
    libgmock-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source files
COPY . .

# Build the project
RUN meson setup builddir --buildtype=release
RUN meson compile -C builddir

# Run tests to ensure quality
RUN meson test -C builddir

# Runtime stage — match builder's glibc (Alpine/musl is incompatible
# with our dynamically linked binary, esp. fortify _chk symbols).
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 \
    netcat-openbsd \
    && rm -rf /var/lib/apt/lists/* \
    && (userdel -r ubuntu 2>/dev/null || true) \
    && groupadd -g 1000 finger \
    && useradd -m -u 1000 -g finger -s /bin/sh finger

# Copy the compiled binary from builder stage
COPY --from=builder /app/builddir/finger /usr/local/bin/finger

# Make the binary executable
RUN chmod +x /usr/local/bin/finger

# Create the directory for user data with proper permissions
RUN mkdir -p /var/finger/users && \
    chown -R finger:finger /var/finger

# Switch to non-root user
USER finger

# Expose port 79 (finger protocol)
EXPOSE 79

# Add health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD nc -w 1 127.0.0.1 79 < /dev/null || exit 1

# Set metadata labels
LABEL org.opencontainers.image.title="finger"
LABEL org.opencontainers.image.description="A silly finger service written in C++20"
LABEL org.opencontainers.image.source="https://github.com/waffle2k/finger"
LABEL org.opencontainers.image.licenses="MIT"

# Set the binary as the default command
CMD ["finger"]
