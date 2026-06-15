# finger

[![CI](https://github.com/waffle2k/finger/workflows/CI/badge.svg)](https://github.com/waffle2k/finger/actions)
[![codecov](https://codecov.io/gh/waffle2k/finger/branch/main/graph/badge.svg)](https://codecov.io/gh/waffle2k/finger)

A silly finger service written in c++20

# Compiling:

```
meson setup builddir
meson compile -C builddir
```

This will create a static linked binary called `builddir/finger`. You can copy this to your remote server if you're going to run it via docker.

# Building a Dockerfile

Create a `Dockerfile` with the contents:

```
# Use a minimal base image
FROM alpine:latest

# Copy the local binary to the container
COPY finger /usr/local/bin/finger

# Make the binary executable
RUN chmod +x /usr/local/bin/finger

# Create the directory for user data
RUN mkdir -p /var/finger/users

# Expose port 79 (finger protocol)
EXPOSE 79

# Set the binary as the default command
CMD ["finger"]
```

And execute `docker build -t finger-app .`

# Running

Create a `docker-compose.yml` file:
```
version: '3.8'

services:
  finger:
    build: .
    ports:
      - "79:79"
    volumes:
      - ./users:/var/finger/users
    restart: unless-stopped
```
and execute `docker compose up -d`

# Setting your status
within the `./users` directory, create a file named after the user you wish to have a response. That's it!

# Abuse protection
Most traffic on port 79 is not finger at all -- HTTP and SIP probes, TLS
handshakes, and username-guessing scanners. None of these resolve to a plan
file, so the daemon treats any request that fails to read a plan as an
"offense" and timestamps it against the source IP. When an IP records more than
3 failures within a rolling 24-hour window, its connections are dropped
(without being read or answered) until those failures age back out of the
window. Legitimate lookups that hit a real plan never count against an IP. All
state is in-memory; thresholds live in `BanTracker::Config` (`ban.hpp`).
