#!/bin/bash
set -e

git clone https://github.com/blender/blender.git blender-test
cd blender-test
git checkout v4.2
make update
./build_files/build_environment/install_deps.sh --with-all

git clone https://github.com/AcademySoftwareFoundation/OpenTimelineIO.git otio-cpp
cd otio-cpp
mkdir build && cd build
cmake .. 
make -j$(nproc)
sudo make install

echo \"Build env ready. cd blender-test && make -j$(nproc) bf_io_otio\"

# COMPLETE bash script.
