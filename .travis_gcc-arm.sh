#!/bin/sh

wget https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q2-update/+download/gcc-arm-none-eabi-4_9-2015q2-20150609-linux.tar.bz2

tar -xjf gcc-arm-none-eabi-4_9-2015q2-20150609-linux.tar.bz2
cd ./gcc-arm-none-eabi-4_9-2015q2/src
find -name ’*.tar.*’ | xargs -I% tar -xf %

cd ../

#Start building the toolchain.
#Can specify "--skip_steps=mingw32" option to skip building windows host
#toolchain, and if specify that option when building prerequisites,
#you have to specify it when building toolchain too.
#
#Without option --build_tools, the tools in current PATH will be used.
#You can download and deploy the provided prebuilt one, use it like
#./build-prerequisites.sh --build_tools=/home/build/prebuilt-native-tools
# or ./build-toolchain.sh --build_tools=/home/build/prebuilt-native-tools
./build-prerequisites.sh 
./build-toolchain.sh 

cd ..
