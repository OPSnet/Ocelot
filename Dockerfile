FROM debian:bookworm-slim

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
        netcat-traditional \
        pkg-config \
    && rm -rf /var/lib/apt/lists/*

COPY . /srv
WORKDIR /srv

RUN mkdir build \
    && cd build \
    && cmake .. \
    && make \
    && apt-get purge -y \
        build-essential \
        cmake \
        pkg-config \
    && apt-get autoremove -y \
    && apt-get clean -y \
    && mv /srv/build/ocelot /srv/ocelot \
    && mv /srv/ocelot.conf.dist /srv/ocelot.conf

# default listen_port value in ocelot.conf
EXPOSE 34000

CMD ["/srv/ocelot"]
