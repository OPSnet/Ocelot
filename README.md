# Ocelot

Ocelot is a BitTorrent tracker written in C++ for the [Gazelle](http://github.com/OPSnet/Gazelle) project.
It supports requests over TCP and can only track IPv4 peers.

## Ocelot Compile-time Dependencies

* [GCC/G++](http://gcc.gnu.org/) (4.7+ required; 4.8.1+ recommended)
* [Boost](http://www.boost.org/) (1.55.0+ required)
* [libev](http://software.schmorp.de/pkg/libev.html) (required)
* [spdlog](https://github.com/gabime/spdlog) (required)
* [MySQL++](http://tangentsoft.net/mysql++/) (3.2.0+ required)
* [jemalloc](http://jemalloc.net/) (optional, but strongly recommended)
* [TCMalloc](http://goog-perftools.sourceforge.net/doc/tcmalloc.html) (optional)

## Installation

### Debian Stretch
```bash
sudo apt-get install \
    build-essential \
    cmake \
    default-libmysqlclient-dev \
    libboost-iostreams-dev \
    libboost-system-dev \
    libev-dev \
    libjemalloc-dev \
    libmysql++-dev \
    pkg-config
mkdir build
cd build
cmake ..
make
```

The [Gazelle installation guides](https://github.com/WhatCD/Gazelle/wiki/Gazelle-installation) include instructions for installing Ocelot as a part of the Gazelle project.

### Standalone Installation

* Create the following tables according to the [Gazelle database schema](https://raw.githubusercontent.com/WhatCD/Gazelle/master/gazelle.sql):
 - `torrents`
 - `torrents_leech_stats`
 - `users_freeleeches`
 - `users_leech_stats`
 - `users_main`
 - `xbt_client_whitelist`
 - `xbt_files_users`
 - `xbt_snatched`

* Edit `ocelot.conf` to your liking.

* Build Ocelot:

        mkdir build
        cd build
        cmake ..
        make

## Running Ocelot

### Run-time options:

* `-c <path/to/ocelot.conf>` - Path to config file. If unspecified, the current working directory is used.
* `-v` - Print queue status every time a flush is initiated.

### Signals

* `SIGHUP` - Reload config
* `SIGUSR1` - Reload torrent list, user list and client whitelist
