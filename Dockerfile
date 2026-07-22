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

FROM ghcr.io/llnl/radiuss:hip-6.4.3-ubuntu-24.04 AS rocm6_4_3
ENV GTEST_COLOR=1
ENV HCC_AMDGPU_TARGET=gfx900
ENV LD_LIBRARY_PATH=/opt/rocm-6.4.3/lib:${LD_LIBRARY_PATH}
COPY . /home/camp/workspace
WORKDIR /home/camp/workspace/build
RUN cmake -G Ninja -B build \
      -DCMAKE_CXX_COMPILER=/opt/rocm-6.4.3/bin/amdclang++ \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DENABLE_WARNINGS=On \
      -DROCM_PATH=/opt/rocm-6.4.3 \
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

FROM ghcr.io/llnl/radiuss:ubuntu-24.04-intel-2024.2 AS sycl
ENV GTEST_COLOR=1
COPY . /home/camp/workspace
WORKDIR /home/camp/workspace/build
RUN /bin/bash -lc 'source /opt/intel/oneapi/setvars.sh 2>&1 && \
    export PATH=/opt/intel/oneapi/compiler/2024.2/bin/:$PATH && \
    export LD_LIBRARY_PATH=/opt/intel/oneapi/2024.2/lib:$LD_LIBRARY_PATH && \
    cmake -G Ninja -B build \
      -DCMAKE_CXX_COMPILER=icpx \
      -DCMAKE_CXX_FLAGS="-fsycl -fsycl-unnamed-lambda" \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DENABLE_WARNINGS=On \
      -DENABLE_SYCL=On .. && \
    cmake --build build --verbose --parallel 4 && \
    cd build && ctest -E "(.*offload|blt.*smoke|cuda_.*|hip_.*|CampEvent|CampResource)" -T test -V'

