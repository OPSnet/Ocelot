# -*- mode: ruby -*-
# vi: set ft=ruby :

$script = <<SCRIPT
sudo apt-get -y install build-essential autoconf
sudo apt-get -y install libboost-iostreams-dev libboost-system-dev
sudo apt-get -y install libev-dev
sudo apt-get -y install libmysqlclient-dev libmysql++-dev
SCRIPT

Vagrant.configure("2") do |config|
  config.vm.box = "debian/contrib-jessie64"

  config.vm.synced_folder ".", "/vagrant"
  config.vm.provision "shell", inline: $script

  config.vbguest.installer_arguments = ['--nox11 -- --force']
end
