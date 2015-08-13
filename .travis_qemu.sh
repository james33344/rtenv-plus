
wget http://wiki.qemu-project.org/download/qemu-2.4.0.tar.bz2


tar -xjf qemu-2.4.0.tar.bz2
cd ./qemu-2.4.0/
./configure --target-list=arm-softmmu,arm-linux-user

make -j2
sudo make install

cd ..
