###############################################################################
# Copyright (c) Lawrence Livermore National Security, LLC and other
# Camp Project Developers. See top-level LICENSE and COPYRIGHT
# files for dates and other details. No copyright assignment is required
# to contribute to Camp.
#
# SPDX-License-Identifier: (BSD-3-Clause)
###############################################################################

ARG BASE_IMG=gcc
ARG COMPILER=g++
ARG VER=latest
ARG PRE_CMD="true"
ARG BUILD_TYPE=RelWithDebInfo
ARG CTEST_EXTRA="-E '(.*offload|blt.*smoke)'"
ARG CTEST_OPTIONS="${CTEST_EXTRA} -T test -V "
ARG CMAKE_EXTRA=""
ARG CMAKE_OPTIONS="-G Ninja -B build ${CMAKE_EXTRA} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DENABLE_WARNINGS=On"
ARG PARALLEL=4
ARG BUILD_EXTRA=""
ARG CMAKE_BUILD_OPTS="--build build --verbose --parallel ${PARALLEL} ${BUILD_EXTRA}"
ARG CUDA_IMG_SUFFIX="-devel-ubuntu22.04"

### start compiler base images ###
# there is no official container in the hub, but there is an official script
# to install clang/llvm by version, installs a bit more than we need, but we
# do not have to maintain it, so I'm alright with that
FROM ghcr.io/llnl/radiuss:clang-${VER}-ubuntu-24.04 AS clang
ENV LD_LIBRARY_PATH=/opt/view/lib

FROM ghcr.io/llnl/radiuss:gcc-${VER}-ubuntu-24.04 AS gcc

FROM nvidia/cuda:${VER}${CUDA_IMG_SUFFIX} AS nvcc

FROM nvcr.io/nvidia/nvhpc:23.3-devel-cuda12.0-ubuntu24.04 AS nvhpc

FROM rocm/dev-ubuntu-24.04:${VER} AS rocm
ENV LD_LIBRARY_PATH=/opt/rocm/lib:${LD_LIBRARY_PATH}

# The intel-runtime container no longer works, use the fat one
FROM intel/oneapi:${VER} AS oneapi
RUN bash -c 'echo . /opt/intel/oneapi/setvars.sh >> ~/setup_env.sh'
### end compiler base images ###

FROM ${BASE_IMG} AS base
ARG VER
ARG BASE_IMG
ENV GTEST_COLOR=1
# Only install CMake/Ninja for images that don't reliably include them.
# (gcc/clang radiuss images already provide a full toolchain.)
COPY scripts/get-deps.sh /tmp/get-deps.sh
RUN /bin/bash -lc 'if [[ "${BASE_IMG}" == "nvcc" || "${BASE_IMG}" == "rocm" || "${BASE_IMG}" == "oneapi" ]]; then bash /tmp/get-deps.sh; fi'
COPY . /home/camp/workspace
WORKDIR /home/camp/workspace/build

FROM base AS test
ARG PRE_CMD
ARG CTEST_OPTIONS
ARG CMAKE_OPTIONS
ARG CMAKE_BUILD_OPTS
ARG COMPILER
ENV COMPILER=${COMPILER:-g++}
RUN /bin/bash -lc '[[ -f ~/setup_env.sh ]] && source ~/setup_env.sh ; \
  CXX="${COMPILER:-g++}" ; \
  if [[ "${CMAKE_OPTIONS}" == *"sanitize=address"* ]]; then \
    sudo apt-get install libclang-rt-${VER}-dev -fy; \
  fi ; \
  ${PRE_CMD} && cmake ${CMAKE_OPTIONS} -DCMAKE_CXX_COMPILER="${CXX}" ..'
RUN /bin/bash -c "[[ -f ~/setup_env.sh ]] && source ~/setup_env.sh ; ${PRE_CMD} && cmake ${CMAKE_BUILD_OPTS}"
RUN /bin/bash -c "[[ -f ~/setup_env.sh ]] && source ~/setup_env.sh ; ${PRE_CMD} && cd build && ctest ${CTEST_OPTIONS}"

# this is here to stop azure from downloading oneapi for every test
FROM alpine AS download_fast
