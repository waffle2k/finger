# Docker Setup and Deployment

This document describes how to build, run, and deploy the finger service using Docker and GitHub Actions.

## Quick Start

### Using Docker Compose (Recommended)

1. **Clone the repository:**
   ```bash
   git clone https://github.com/waffle2k/finger.git
   cd finger
   ```

2. **Start the service:**
   ```bash
   docker compose up -d
   ```

3. **Test the service:**
   ```bash
   # Test with the example user
   finger john@localhost
   
   # Or using telnet
   telnet localhost 79
   # Then type: john
   ```

4. **Add your own users:**
   ```bash
   # Create a status file for a user
   echo "Your status message here" > users/yourusername
   
   # Test it
   finger yourusername@localhost
   ```

### Using Docker directly

1. **Build the image:**
   ```bash
   docker build -t finger-service .
   ```

2. **Run the container:**
   ```bash
   docker run -d \
     --name finger \
     -p 79:79 \
     -v $(pwd)/users:/var/finger/users \
     finger-service
   ```

### Using Pre-built Images

You can also use the automatically built images from GitHub Container Registry:

```bash
docker run -d \
  --name finger \
  -p 79:79 \
  -v $(pwd)/users:/var/finger/users \
  ghcr.io/waffle2k/finger:latest
```

## Docker Architecture

### Multi-stage Build

The Dockerfile uses a multi-stage build approach:

1. **Builder Stage (Ubuntu 24.04):**
   - Installs all build dependencies (meson, ninja, boost, gtest, etc.)
   - Compiles the C++20 source code
   - Runs all tests to ensure quality
   - Creates a statically linked binary

2. **Runtime Stage (Alpine Linux):**
   - Minimal base image (~5MB)
   - Only includes runtime dependencies
   - Runs as non-root user for security
   - Includes health checks

### Security Features

- **Non-root execution:** Runs as user `finger` (UID 1000)
- **Minimal attack surface:** Alpine Linux base with minimal packages
- **Health checks:** Built-in container health monitoring
- **Read-only filesystem:** Application doesn't write to filesystem

### Image Size

- **Final image:** ~15MB (Alpine + binary + minimal runtime deps)
- **Build image:** ~2GB (includes all build tools, discarded after build)

## GitHub Actions CI/CD

### Automated Workflow

The repository includes a comprehensive GitHub Actions workflow (`.github/workflows/docker-publish.yml`) that:

1. **Build and Test:**
   - Builds the project with meson
   - Runs all unit tests
   - Uploads test results as artifacts

2. **Multi-platform Docker Build:**
   - Builds for `linux/amd64` and `linux/arm64`
   - Uses Docker Buildx for cross-platform support
   - Implements build caching for faster builds

3. **Container Registry Publishing:**
   - Publishes to GitHub Container Registry (`ghcr.io`)
   - Tags with multiple strategies:
     - `latest` for main branch
     - `v1.2.3` for semantic version tags
     - `main-abc1234` for commit SHA
     - `pr-123` for pull requests

4. **Security Scanning:**
   - Runs Trivy vulnerability scanner
   - Uploads results to GitHub Security tab
   - Fails on high-severity vulnerabilities

5. **Supply Chain Security:**
   - Generates SLSA build provenance attestations
   - Signs container images
   - Provides build transparency

### Triggering Builds

The workflow triggers on:
- **Push to main branch:** Builds and publishes `latest` tag
- **Version tags:** Builds and publishes semantic version tags (`v1.0.0`)
- **Pull requests:** Builds but doesn't publish (security)

### Using Published Images

Images are available at: `ghcr.io/waffle2k/finger`

Available tags:
- `latest` - Latest stable build from main branch
- `v1.0.0` - Specific version releases
- `main-abc1234` - Specific commit builds

## Configuration

### Environment Variables

The container supports these environment variables:

- `FINGER_PORT`: Port to listen on (default: 79)
- `FINGER_DATA_DIR`: Directory for user files (default: /var/finger/users)

### Volume Mounts

- `/var/finger/users`: Directory containing user status files
  - Mount your local `users/` directory here
  - Each file represents a user (filename = username)
  - File contents = user's status message

### Health Checks

The container includes built-in health checks:
- **Check:** TCP connection to port 79
- **Interval:** Every 30 seconds
- **Timeout:** 10 seconds
- **Retries:** 3 attempts
- **Start period:** 40 seconds

## Development

### Local Development with Docker

1. **Build development image:**
   ```bash
   docker build --target builder -t finger-dev .
   ```

2. **Run tests in container:**
   ```bash
   docker run --rm finger-dev meson test -C builddir
   ```

3. **Interactive development:**
   ```bash
   docker run -it --rm \
     -v $(pwd):/app \
     -w /app \
     finger-dev bash
   ```

### Debugging

1. **View container logs:**
   ```bash
   docker logs finger
   ```

2. **Execute into running container:**
   ```bash
   docker exec -it finger sh
   ```

3. **Check health status:**
   ```bash
   docker inspect finger | grep -A 10 Health
   ```

## Production Deployment

### Docker Swarm

```yaml
version: '3.8'
services:
  finger:
    image: ghcr.io/waffle2k/finger:latest
    ports:
      - "79:79"
    volumes:
      - finger_data:/var/finger/users
    deploy:
      replicas: 2
      restart_policy:
        condition: on-failure
    healthcheck:
      test: ["CMD", "nc", "-z", "localhost", "79"]
      interval: 30s
      timeout: 10s
      retries: 3

volumes:
  finger_data:
```

### Kubernetes

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: finger-service
spec:
  replicas: 3
  selector:
    matchLabels:
      app: finger
  template:
    metadata:
      labels:
        app: finger
    spec:
      containers:
      - name: finger
        image: ghcr.io/waffle2k/finger:latest
        ports:
        - containerPort: 79
        volumeMounts:
        - name: user-data
          mountPath: /var/finger/users
        livenessProbe:
          tcpSocket:
            port: 79
          initialDelaySeconds: 30
          periodSeconds: 10
      volumes:
      - name: user-data
        configMap:
          name: finger-users
---
apiVersion: v1
kind: Service
metadata:
  name: finger-service
spec:
  selector:
    app: finger
  ports:
  - port: 79
    targetPort: 79
  type: LoadBalancer
```

## Troubleshooting

### Common Issues

1. **Permission denied on user files:**
   ```bash
   # Fix file permissions
   chmod 644 users/*
   ```

2. **Port 79 requires root:**
   ```bash
   # Use a different port
   docker run -p 8079:79 finger-service
   ```

3. **Container won't start:**
   ```bash
   # Check logs
   docker logs finger
   
   # Check if port is available
   netstat -ln | grep :79
   ```

4. **Health check failing:**
   ```bash
   # Test manually
   docker exec finger nc -z localhost 79
   
   # Check if service is running
   docker exec finger ps aux
   ```

### Performance Tuning

1. **Resource limits:**
   ```yaml
   services:
     finger:
       deploy:
         resources:
           limits:
             memory: 64M
             cpus: '0.1'
   ```

2. **Connection limits:**
   - The service handles concurrent connections efficiently
   - Default OS limits should be sufficient for most use cases
   - Monitor with `docker stats` for resource usage

## Security Considerations

1. **Network Security:**
   - Finger protocol sends data in plain text
   - Consider using behind a reverse proxy with TLS
   - Restrict access with firewall rules

2. **Data Security:**
   - User files are readable by the finger user
   - Don't store sensitive information in status files
   - Consider file permissions on the host

3. **Container Security:**
   - Runs as non-root user
   - Uses minimal base image
   - Regular security scanning in CI/CD
   - Keep images updated

## Contributing

When contributing Docker-related changes:

1. Test locally with `docker build`
2. Ensure all tests pass in the container
3. Update this documentation if needed
4. The CI/CD pipeline will automatically test your changes

For more information, see the main [README.md](README.md).
