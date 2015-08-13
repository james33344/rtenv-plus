#!/bin/sh

wget https://launchpad.net/ubuntu/+archive/primary/+files/newlib_2.1.0.orig.tar.gz


tar -xzf newlib_2.1.0.orig.tar.gz
cd ./newlib-2.1.0/

./configure
sudo make
sudo make install

cd ..
