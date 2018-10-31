# -*- mode: ruby -*-
# vi: set ft=ruby :

$script = <<SCRIPT
apt-get update
apt-get -y install \
    autoconf \
    build-essential \
    default-libmysqlclient-dev \
    libboost-iostreams-dev \
    libboost-system-dev \
    libev-dev \
    libmysql++-dev \
    pkg-config \
    mariadb-client \
    mariadb-server 
SCRIPT

Vagrant.configure("2") do |config|
  config.vm.box = "debian/contrib-stretch64"

  config.vm.synced_folder ".", "/vagrant"
  config.vm.provision "shell", inline: $script

  config.vbguest.installer_arguments = ['--nox11 -- --force']
end
