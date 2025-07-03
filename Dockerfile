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

# Runtime stage - minimal Alpine Linux
FROM alpine:latest

# Install runtime dependencies (if any)
RUN apk add --no-cache \
    libstdc++ \
    && addgroup -g 1000 finger \
    && adduser -D -s /bin/sh -u 1000 -G finger finger

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
    CMD nc -z localhost 79 || exit 1

# Set metadata labels
LABEL org.opencontainers.image.title="finger"
LABEL org.opencontainers.image.description="A silly finger service written in C++20"
LABEL org.opencontainers.image.source="https://github.com/waffle2k/finger"
LABEL org.opencontainers.image.licenses="MIT"

# Set the binary as the default command
CMD ["finger"]
