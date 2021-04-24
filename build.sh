#!/bin/sh

rm -rf build
mkdir build
cd build

if [ $# -gt 0 ]
then 
  echo "Making custom build $1..."
  cmake -DCMAKE_BUILD_TYPE=$1 ..
else
  echo "Making release..."
  cmake ..
fi
make -j 4
cd ..
echo "Done"
