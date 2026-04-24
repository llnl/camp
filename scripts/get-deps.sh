#!/bin/bash -e

###############################################################################
# Copyright (c) 2018-25, Lawrence Livermore National Security, LLC
# and Camp project contributors. See the camp/LICENSE file for details.
#
# SPDX-License-Identifier: (BSD-3-Clause)
###############################################################################

apt-get update || true #ignore fail here, because rocm docker is broken
apt-get install -y --no-install-recommends curl unzip sudo
apt-get clean
rm -rf /var/lib/apt/lists/*

CMAKE=3.26.4
mkdir installers
pushd installers
curl -s -L https://github.com/Kitware/CMake/releases/download/v$CMAKE/cmake-$CMAKE-linux-$(uname -m).sh > cmake.sh
bash cmake.sh --prefix=/usr/local --skip-license

curl -s -L https://github.com/ninja-build/ninja/releases/download/v1.10.2/ninja-linux.zip > ninja.zip
unzip ninja.zip
mv ninja /usr/local/bin
popd

rm -rf installers
