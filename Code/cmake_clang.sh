#use this script if you want Cmake to use clang
#pass 0/1 to disable/enable debug functionality

export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
export LD=/usr/bin/llvm-ld

cmake -S . -B build -DDEBUG=$1

cd build
make
