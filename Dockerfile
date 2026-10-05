# Someone complain about cannot using 2.38;v
FROM ubuntu:mantic AS build
 
ARG APP_NAME=lyla
 
RUN for f in /etc/apt/sources.list /etc/apt/sources.list.d/*; do \
      [ -f "$f" ] && sed -i \
        -e 's|http://archive.ubuntu.com/ubuntu|http://old-releases.ubuntu.com/ubuntu|g' \
        -e 's|http://security.ubuntu.com/ubuntu|http://old-releases.ubuntu.com/ubuntu|g' \
        -e 's|http://ports.ubuntu.com/ubuntu-ports|http://old-releases.ubuntu.com/ubuntu|g' \
        "$f"; \
    done; true
 
RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential meson ninja-build pkg-config git ca-certificates cmake \
      libgl-dev libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev \
      libwayland-dev libxkbcommon-dev wayland-protocols \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY meson.build ./
COPY meson.options ./
COPY README.md ./
COPY LICENSE ./
COPY CREDITS.txt ./
COPY src/ ./src/
COPY external/ ./external/
COPY subprojects/ ./subprojects/

RUN meson setup build --buildtype=release --strip \
 && meson compile -C build \
 && cp build/src/${APP_NAME} /app
 
FROM ubuntu:mantic AS runtime
 
RUN for f in /etc/apt/sources.list /etc/apt/sources.list.d/*; do \
      [ -f "$f" ] && sed -i \
        -e 's|http://archive.ubuntu.com/ubuntu|http://old-releases.ubuntu.com/ubuntu|g' \
        -e 's|http://security.ubuntu.com/ubuntu|http://old-releases.ubuntu.com/ubuntu|g' \
        -e 's|http://ports.ubuntu.com/ubuntu-ports|http://old-releases.ubuntu.com/ubuntu|g' \
        "$f"; \
    done; true
 
RUN apt-get update && apt-get install -y --no-install-recommends \
      libgl1 libx11-6 libxrandr2 libxi6 libxcursor1 libxinerama1 \
      libglx-mesa0 libgl1-mesa-dri binutils \
 && rm -rf /var/lib/apt/lists/*
 
COPY --from=build /app /usr/local/bin/app

RUN echo "===== glibc version in this image =====" \
 && ldd --version | head -n 1 \
 && echo "===== ldd /usr/local/bin/app =====" \
 && ldd /usr/local/bin/app \
 && echo "===== highest GLIBC symbol version the binary needs =====" \
 && (objdump -T /usr/local/bin/app | grep -o 'GLIBC_[0-9.]*' | sort -uV | tail -n 1) \
 && echo "===== end of report ====="
 
RUN useradd -m appuser
USER appuser
 
ENTRYPOINT ["app"]
 
FROM scratch AS export
COPY --from=build /app /app
 
