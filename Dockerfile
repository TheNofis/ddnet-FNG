FROM alpine:3.20.3 AS build

RUN apk add --no-cache \
        cargo \
        cmake \
        curl-dev \
        g++ \
        gcc \
        libpng-dev \
        libwebsockets-dev \
        mariadb-dev \
        ninja \
        python3 \
        sqlite-dev

COPY . /tw/sources

WORKDIR /tw/build

RUN cmake /tw/sources \
        -Wno-dev \
    -DCMAKE_INSTALL_PREFIX=/tw/install \
        -DAUTOUPDATE=OFF \
        -DCLIENT=OFF \
        -DDEV=OFF \
        -DIPO=ON \
        -DMYSQL=ON \
        -DPREFER_BUNDLED_LIBS=OFF \
        -DVIDEORECORDER=OFF \
        -DVULKAN=OFF \
        -DWEBSOCKETS=ON \
        -GNinja

RUN cmake --build . -t install

# ---

FROM alpine:3.20.3 AS ddnet

RUN apk add --no-cache \
        libcurl \
        libstdc++ \
        libwebsockets \
        mariadb-connector-c \
        sqlite-libs

COPY --from=build /tw/install/bin /tw/bin
COPY --from=build /tw/install/share/ddnet/data/maps /tw/data/maps
COPY --from=build /tw/install/lib /tw/lib

WORKDIR /tw/data
ENTRYPOINT ["/tw/bin/DDNet-Server"]