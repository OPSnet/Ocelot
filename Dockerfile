FROM debian:stretch-slim

LABEL org.opencontainers.image.source=https://github.com/OPSnet/Ocelot

RUN apt-get update \
    && apt-get install --no-install-recommends -y \
        build-essential \
        cmake \
        curl \
        default-libmysqlclient-dev \
        libboost-iostreams-dev \
        libboost-system-dev \
        libev-dev \
        libjemalloc-dev \
        libmysql++-dev \
        pkg-config

COPY . /srv
WORKDIR /srv

RUN mkdir build \
    && cd build \
    && cmake .. \
    && make \
    && mv /srv/build/ocelot /srv/ocelot \
    && mv /srv/ocelot.conf.dist /srv/ocelot.conf

RUN apt-get purge -y \
        build-essential \
        cmake \
        pkg-config \
    && apt-get autoremove -y \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

# default listen_port value in ocelot.conf
EXPOSE 34000

CMD ["/srv/ocelot"]
