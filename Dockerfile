FROM debian:bookworm-slim

# Use USTC mirror
RUN sed -i \
    's|deb.debian.org/debian|mirrors.ustc.edu.cn/debian|g; \
     s|security.debian.org/debian-security|mirrors.ustc.edu.cn/debian-security|g' \
    /etc/apt/sources.list.d/debian.sources

# Install build tools and MinGW-w64 x86_64 toolchain
RUN for i in 1 2 3 4 5; do \
        apt-get update && \
        apt-get install -y --no-install-recommends \
            ca-certificates \
            git \
            cmake \
            ninja-build \
            gcc-mingw-w64-x86-64-posix \
            g++-mingw-w64-x86-64-posix \
            binutils-mingw-w64-x86-64 \
            mingw-w64-x86-64-dev \
        && break \
        || { echo "apt install failed (attempt $i), retrying..."; sleep 5; }; \
    done \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

COPY toolchains/x86_64-w64-mingw32.cmake /opt/toolchains/mingw-x64.cmake

WORKDIR /workspace

CMD ["/bin/bash"]