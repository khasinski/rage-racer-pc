FROM debian:trixie-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends gcc cmake ninja-build libsdl3-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /source
COPY CMakeLists.txt hd_server.c ./
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
FROM debian:trixie-slim
RUN apt-get update && apt-get install -y --no-install-recommends libsdl3-0 && rm -rf /var/lib/apt/lists/*
COPY --from=build /source/build/rage-hd-server /usr/local/bin/
ENV HD_BIND=0.0.0.0
USER 65534:65534
EXPOSE 3000
ENTRYPOINT ["rage-hd-server", "/releases", "3000"]
