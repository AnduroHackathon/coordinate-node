    # Update new packages
    FROM ubuntu:22.04

    LABEL maintainer="michael.casey@mara.com"
    LABEL version="0.1"
    LABEL description="Docker file for anduro"

    ARG DEBIAN_FRONTEND=noninteractive

    RUN apt-get update
    RUN apt-get install -y \
        build-essential \
        libtool \
        autotools-dev \
        automake \
        pkg-config \
        bsdmainutils \
        python3 \
        libevent-dev \
        libboost-dev \
        libsqlite3-dev \
        libminiupnpc-dev \
        libnatpmp-dev \
        libzmq3-dev \
        systemtap-sdt-dev \
        curl 


    #Setup base directory
    RUN mkdir -p /opt/coordinate
    WORKDIR /opt/coordinate

    COPY . ./

    #Configure sidechain node
    WORKDIR /opt/coordinate
    RUN ./autogen.sh \
     && ./configure --without-gui \
     && make

    ENTRYPOINT ["./src/coordinated"]

    CMD []
