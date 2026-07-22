###############################################################################
# Copyright (c) Lawrence Livermore National Security, LLC and other
# Camp Project Developers. See top-level LICENSE and COPYRIGHT
# files for dates and other details. No copyright assignment is required
# to contribute to Camp.
#
# SPDX-License-Identifier: (BSD-3-Clause)
###############################################################################

FROM ghcr.io/llnl/radiuss:gcc-11-ubuntu-24.04 AS gcc11_debug
ENV GTEST_COLOR=1
COPY . /home/camp/workspace
WORKDIR /home/camp/workspace/build
RUN cmake -G Ninja -B build \
      -DCMAKE_CXX_COMPILER=g++ \
      -DCMAKE_BUILD_TYPE=Debug \
      -DENABLE_WARNINGS=On \
      -DENABLE_WARNINGS_AS_ERRORS=On \
      -DENABLE_COVERAGE=On .. && \
    cmake --build build --verbose --parallel 4 && \
    cd build && ctest -E '(.*offload|blt.*smoke|cuda_.*|hip_.*|CampEvent|CampResource)' -T test -V

FROM ghcr.io/llnl/radiuss:gcc-11-ubuntu-24.04 AS gcc11
ENV GTEST_COLOR=1
COPY . /home/camp/workspace
WORKDIR /home/camp/workspace/build
RUN cmake -G Ninja -B build \
      -DCMAKE_CXX_COMPILER=g++ \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DENABLE_WARNINGS=On .. && \
    cmake --build build --verbose --parallel 4 && \
    cd build && ctest -E '(.*offload|blt.*smoke|cuda_.*|hip_.*|CampEvent|CampResource)' -T test -V

FROM ghcr.io/llnl/radiuss:gcc-14-ubuntu-24.04 AS gcc14
ENV GTEST_COLOR=1
COPY . /home/camp/workspace
WORKDIR /home/camp/workspace/build
RUN cmake -G Ninja -B build \
      -DCMAKE_CXX_COMPILER=g++ \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DENABLE_WARNINGS=On .. && \
    cmake --build build --verbose --parallel 4 && \
    cd build && ctest -E '(.*offload|blt.*smoke|cuda_.*|hip_.*|CampEvent|CampResource)' -T test -V

FROM ghcr.io/llnl/radiuss:clang-19-ubuntu-24.04 AS clang19
ENV GTEST_COLOR=1
ENV LD_LIBRARY_PATH=/opt/view/lib
COPY . /home/camp/workspace
WORKDIR /home/camp/workspace/build
RUN cmake -G Ninja -B build \
      -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DENABLE_WARNINGS=On .. && \
    cmake --build build --verbose --parallel 4 && \
    cd build && ctest -E '(.*offload|blt.*smoke|cuda_.*|hip_.*|CampEvent|CampResource)' -T test -V

FROM nvidia/cuda:12.0.0-devel-ubuntu22.04 AS nvcc12
ENV GTEST_COLOR=1
COPY scripts/get-deps.sh /tmp/get-deps.sh
RUN bash /tmp/get-deps.sh
COPY . /home/camp/workspace
WORKDIR /home/camp/workspace/build
RUN cmake -G Ninja -B build \
      -DCMAKE_CXX_COMPILER=g++ \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DENABLE_WARNINGS=On \
      -DENABLE_CUDA=On \
      -DCMAKE_CUDA_ARCHITECTURES=70 .. && \
    cmake --build build --verbose --parallel 4 && \
    cd build && ctest -E '(.*offload|blt.*smoke|cuda_.*|hip_.*|CampEvent|CampResource)' -T test -V

FROM nvidia/cuda:12.2.2-devel-ubuntu22.04 AS nvcc12_debug
ENV GTEST_COLOR=1
COPY scripts/get-deps.sh /tmp/get-deps.sh
RUN bash /tmp/get-deps.sh
COPY . /home/camp/workspace
WORKDIR /home/camp/workspace/build
RUN cmake -G Ninja -B build \
      -DCMAKE_CXX_COMPILER=g++ \
      -DCMAKE_BUILD_TYPE=Debug \
      -DENABLE_WARNINGS=On \
      -DENABLE_CUDA=On \
      -DCMAKE_CUDA_ARCHITECTURES=70 .. && \
    cmake --build build --verbose --parallel 4 && \
    cd build && ctest -E '(.*offload|blt.*smoke|cuda_.*|hip_.*|CampEvent|CampResource)' -T test -V

FROM rocm/dev-ubuntu-24.04:6.4.3 AS rocm6_4_3
ENV GTEST_COLOR=1
ENV LD_LIBRARY_PATH=/opt/rocm/lib:${LD_LIBRARY_PATH}
COPY scripts/get-deps.sh /tmp/get-deps.sh
RUN bash /tmp/get-deps.sh
COPY . /home/camp/workspace
WORKDIR /home/camp/workspace/build
RUN cmake -G Ninja -B build \
      -DCMAKE_CXX_COMPILER=/opt/rocm/llvm/bin/amdclang++ \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DENABLE_WARNINGS=On \
      -DROCM_PATH=/opt/rocm \
      -DENABLE_HIP=On \
      -DENABLE_OPENMP=Off \
      -DENABLE_CUDA=Off .. && \
    cmake --build build --verbose --parallel 4 && \
    cd build && ctest -E '(.*offload|blt.*smoke|cuda_.*|hip_.*|CampEvent|CampResource)' -T test -V

FROM rocm/dev-ubuntu-24.04:latest AS rocm_latest
ENV GTEST_COLOR=1
ENV LD_LIBRARY_PATH=/opt/rocm/lib:${LD_LIBRARY_PATH}
COPY scripts/get-deps.sh /tmp/get-deps.sh
RUN bash /tmp/get-deps.sh
COPY . /home/camp/workspace
WORKDIR /home/camp/workspace/build
RUN cmake -G Ninja -B build \
      -DCMAKE_CXX_COMPILER=/opt/rocm/llvm/bin/amdclang++ \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DENABLE_WARNINGS=On \
      -DROCM_PATH=/opt/rocm \
      -DENABLE_HIP=On \
      -DENABLE_OPENMP=Off \
      -DENABLE_CUDA=Off .. && \
    cmake --build build --verbose --parallel 4 && \
    cd build && ctest -E '(.*offload|blt.*smoke|cuda_.*|hip_.*|CampEvent|CampResource)' -T test -V

FROM intel/oneapi:latest AS sycl
ENV GTEST_COLOR=1
RUN bash -c 'echo . /opt/intel/oneapi/setvars.sh >> ~/setup_env.sh'
COPY scripts/get-deps.sh /tmp/get-deps.sh
RUN bash /tmp/get-deps.sh
COPY . /home/camp/workspace
WORKDIR /home/camp/workspace/build
RUN /bin/bash -lc 'source ~/setup_env.sh && \
    cmake -G Ninja -B build \
      -DCMAKE_CXX_COMPILER=dpcpp \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DENABLE_WARNINGS=On \
      -DENABLE_SYCL=On .. && \
    cmake --build build --verbose --parallel 4 && \
    cd build && ctest -E '(.*offload|blt.*smoke|cuda_.*|hip_.*|CampEvent|CampResource)' -T test -V'

#ARG BASE_IMG=gcc
#ARG COMPILER=g++
#ARG VER=latest
#ARG PRE_CMD="true"
#ARG BUILD_TYPE=RelWithDebInfo
#ARG CTEST_EXTRA="-E '(.*offload|blt.*smoke)'"
#ARG CTEST_OPTIONS="${CTEST_EXTRA} -T test -V "
#ARG CMAKE_EXTRA=""
#ARG CMAKE_OPTIONS="-G Ninja -B build ${CMAKE_EXTRA} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DENABLE_WARNINGS=On"
#ARG PARALLEL=4
#ARG BUILD_EXTRA=""
#ARG CMAKE_BUILD_OPTS="--build build --verbose --parallel ${PARALLEL} ${BUILD_EXTRA}"
#ARG CUDA_IMG_SUFFIX="-devel-ubuntu22.04"
#
#### start compiler base images ###
## there is no official container in the hub, but there is an official script
## to install clang/llvm by version, installs a bit more than we need, but we
## do not have to maintain it, so I'm alright with that
#FROM ghcr.io/llnl/radiuss:clang-${VER}-ubuntu-24.04 AS clang
#ENV LD_LIBRARY_PATH=/opt/view/lib
#
#FROM ghcr.io/llnl/radiuss:gcc-${VER}-ubuntu-24.04 AS gcc
#
#FROM nvidia/cuda:${VER}${CUDA_IMG_SUFFIX} AS nvcc
#
#FROM nvcr.io/nvidia/nvhpc:23.3-devel-cuda12.0-ubuntu24.04 AS nvhpc
#
#FROM rocm/dev-ubuntu-24.04:${VER} AS rocm
#ENV LD_LIBRARY_PATH=/opt/rocm/lib:${LD_LIBRARY_PATH}
#
## The intel-runtime container no longer works, use the fat one
#FROM intel/oneapi:${VER} AS oneapi
#RUN bash -c 'echo . /opt/intel/oneapi/setvars.sh >> ~/setup_env.sh'
#### end compiler base images ###
#
#FROM ${BASE_IMG} AS base
#ARG VER
#ARG BASE_IMG
#ENV GTEST_COLOR=1
## Only install CMake/Ninja for images that don't reliably include them.
## (gcc/clang radiuss images already provide a full toolchain.)
#COPY scripts/get-deps.sh /tmp/get-deps.sh
#RUN /bin/bash -lc 'if [[ "${BASE_IMG}" == "nvcc" || "${BASE_IMG}" == "rocm" || "${BASE_IMG}" == "oneapi" ]]; then bash /tmp/get-deps.sh; fi'
#COPY . /home/camp/workspace
#WORKDIR /home/camp/workspace/build
#
#FROM base AS test
#ARG PRE_CMD
#ARG CTEST_OPTIONS
#ARG CMAKE_OPTIONS
#ARG CMAKE_BUILD_OPTS
#ARG COMPILER
#ENV COMPILER=${COMPILER:-g++}
#RUN /bin/bash -lc '[[ -f ~/setup_env.sh ]] && source ~/setup_env.sh ; \
#  CXX="${COMPILER:-g++}" ; \
#  if [[ "${CMAKE_OPTIONS}" == *"sanitize=address"* ]]; then \
#    sudo apt-get install libclang-rt-${VER}-dev -fy; \
#  fi ; \
#  ${PRE_CMD} && cmake ${CMAKE_OPTIONS} -DCMAKE_CXX_COMPILER="${CXX}" ..'
#RUN /bin/bash -c "[[ -f ~/setup_env.sh ]] && source ~/setup_env.sh ; ${PRE_CMD} && cmake ${CMAKE_BUILD_OPTS}"
#RUN /bin/bash -c "[[ -f ~/setup_env.sh ]] && source ~/setup_env.sh ; ${PRE_CMD} && cd build && ctest ${CTEST_OPTIONS}"
#
## this is here to stop azure from downloading oneapi for every test
#FROM alpine AS download_fast
