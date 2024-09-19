#!/usr/bin/env bash

set -euxo pipefail

sudo -v

git clone https://github.com/jbrhm/OrientationVisualizer.git

cd ~/OrientationVisualizer

cmake . -B build
cmake --build build
sudo cmake --install build
