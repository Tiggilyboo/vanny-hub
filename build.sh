#!/bin/sh

rm -rf build
rm -f compile_commands.json

mkdir build
cd build


if [ $# -gt 0 ]
then
  echo "Making custom build $1..."
  cmake -DCMAKE_BUILD_TYPE=$1 -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..
else
  echo "Making release..."
  cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..
fi

make -j4

cd ..

ln -s build/compile_commands.json compile_commands.json

echo "Done"
