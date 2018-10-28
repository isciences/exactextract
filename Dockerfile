FROM isciences/exactextract-build-env:latest

LABEL maintainer="dbaston@isciences.com"

COPY . /exactextract

RUN mkdir /cmake-build-release && \
    cd /cmake-build-release && \
    cmake -DCMAKE_BUILD_TYPE=Release /exactextract && \
    make && \
    ./catch_tests && \
    make install && \
    rm -rf /cmake-build-release

ENTRYPOINT exactextract
