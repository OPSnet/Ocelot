# -*- mode: ruby -*-
# vi: set ft=ruby :

$script = <<SCRIPT
sudo apt-get -y install build-essential autoconf
sudo apt-get -y install libboost-iostreams-dev libboost-system-dev
sudo apt-get -y install libev-dev
sudo apt-get -y install libmysqlclient-dev libmysql++-dev

debconf-set-selections <<< 'mariadb-server mysql-server/root_password password em%G9Lrey4^N'
debconf-set-selections <<< 'mariadb-server mysql-server/root_password_again password em%G9Lrey4^N'
sudo apt-get install -y mariadb-server mariadb-client

mysql -uroot -pem%G9Lrey4^N < /vagrant/gazelle.sql
mysql -uroot -pem%G9Lrey4^N -e "CREATE USER 'gazelle'@'%' IDENTIFIED BY 'password';"
mysql -uroot -pem%G9Lrey4^N -e "GRANT ALL ON *.* TO 'gazelle'@'%';"
mysql -uroot -pem%G9Lrey4^N -e "FLUSH PRIVILEGES;"
sudo sed -i "s/^bind-address/\# bind-address/" /etc/mysql/my.cnf
sudo sed -i "s/^skip-external-locking/\# skip-external-locking/" /etc/mysql/my.cnf

sudo service mysql restart

SCRIPT

Vagrant.configure("2") do |config|
  config.vm.box = "debian/contrib-jessie64"

  config.vm.network :forwarded_port, guest: 3306, host: 36000
  config.vm.network :forwarded_port, guest: 34000, host: 34000

  config.vm.synced_folder ".", "/vagrant"
  config.vm.provision "shell", inline: $script

  config.vbguest.installer_arguments = ['--nox11 -- --force']
end
