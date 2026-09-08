//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2018-25, Lawrence Livermore National Security, LLC
// and Camp project contributors. See the camp/LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef CAMP_TEST_HPP
#define CAMP_TEST_HPP

#include "camp/defines.hpp"
#include "gtest/gtest.h"

// TODO: Add set up and tear down macros
//       For CUDA and HIP set up, create a stream

///
/// Host test set up macro
///
#define CAMP_HOST_TEST(CAMP_SUITE_NAME, CAMP_TEST_NAME)                 \
  namespace camp                                                        \
  {                                                                     \
  namespace test                                                        \
  {                                                                     \
    namespace CAMP_SUITE_NAME                                           \
    {                                                                   \
      void CAMP_TEST_NAME##_host_kernel(bool* passed)                   \
      {                                                                 \
        *passed = CAMP_TEST_NAME();                                     \
      }                                                                 \
    }                                                                   \
  }                                                                     \
  }                                                                     \
                                                                        \
  TEST(host_##CAMP_SUITE_NAME, CAMP_TEST_NAME)                          \
  {                                                                     \
    bool passed;                                                        \
    camp::test::CAMP_SUITE_NAME::CAMP_TEST_NAME##_host_kernel(&passed); \
    EXPECT_TRUE(passed);                                                \
  }

///
/// CUDA test set up macro
///
#if defined(CAMP_ENABLE_CUDA) && defined(__CUDACC__)
#define CAMP_CUDA_TEST(CAMP_SUITE_NAME, CAMP_TEST_NAME)                    \
  namespace camp                                                           \
  {                                                                        \
  namespace test                                                           \
  {                                                                        \
    namespace CAMP_SUITE_NAME                                              \
    {                                                                      \
      __global__ void CAMP_TEST_NAME##_cuda_kernel(bool* passed)           \
      {                                                                    \
        *passed = CAMP_TEST_NAME();                                        \
      }                                                                    \
    }                                                                      \
  }                                                                        \
  }                                                                        \
                                                                           \
  TEST(cuda_##CAMP_SUITE_NAME, CAMP_TEST_NAME)                             \
  {                                                                        \
    bool passed = false;                                                   \
    bool* buffer;                                                          \
    auto error = cudaMallocHost((void**)&buffer, sizeof(bool));            \
                                                                           \
    if (error == cudaSuccess) {                                            \
      camp::test::CAMP_SUITE_NAME::CAMP_TEST_NAME##_cuda_kernel<<<1, 1>>>( \
          buffer);                                                         \
      error = cudaDeviceSynchronize();                                     \
                                                                           \
      if (error == cudaSuccess) {                                          \
        passed = *buffer;                                                  \
        error = cudaFreeHost(buffer);                                      \
                                                                           \
        if (error != cudaSuccess) {                                        \
          passed = false;                                                  \
        }                                                                  \
      }                                                                    \
    }                                                                      \
                                                                           \
    EXPECT_TRUE(passed);                                                   \
  }
#else
#define CAMP_CUDA_TEST(CAMP_SUITE_NAME, CAMP_TEST_NAME)
#endif

///
/// HIP test set up macro
///
#if defined(CAMP_ENABLE_HIP) && defined(__HIPCC__)
#define CAMP_HIP_TEST(CAMP_SUITE_NAME, CAMP_TEST_NAME)                    \
  namespace camp                                                          \
  {                                                                       \
  namespace test                                                          \
  {                                                                       \
    namespace CAMP_SUITE_NAME                                             \
    {                                                                     \
      __global__ void CAMP_TEST_NAME##_hip_kernel(bool* passed)           \
      {                                                                   \
        *passed = CAMP_TEST_NAME();                                       \
      }                                                                   \
    }                                                                     \
  }                                                                       \
  }                                                                       \
                                                                          \
  TEST(hip_##CAMP_SUITE_NAME, CAMP_TEST_NAME)                             \
  {                                                                       \
    bool passed = false;                                                  \
    bool* buffer;                                                         \
    auto error = hipHostMalloc((void**)&buffer, sizeof(bool));            \
                                                                          \
    if (error == hipSuccess) {                                            \
      camp::test::CAMP_SUITE_NAME::CAMP_TEST_NAME##_hip_kernel<<<1, 1>>>( \
          buffer);                                                        \
      error = hipDeviceSynchronize();                                     \
                                                                          \
      if (error == hipSuccess) {                                          \
        passed = *buffer;                                                 \
        error = hipHostFree(buffer);                                      \
                                                                          \
        if (error != hipSuccess) {                                        \
          passed = false;                                                 \
        }                                                                 \
      }                                                                   \
    }                                                                     \
                                                                          \
    EXPECT_TRUE(passed);                                                  \
  }
#else
#define CAMP_HIP_TEST(CAMP_SUITE_NAME, CAMP_TEST_NAME)
#endif

///
/// Macros to test all enabled programming models
///
#define CAMP_TEST_BEGIN(CAMP_SUITE_NAME, CAMP_TEST_NAME) \
  namespace camp                                         \
  {                                                      \
  namespace test                                         \
  {                                                      \
    namespace CAMP_SUITE_NAME                            \
    {                                                    \
      CAMP_HOST_DEVICE bool CAMP_TEST_NAME()             \
      {

#define CAMP_TEST_END(CAMP_SUITE_NAME, CAMP_TEST_NAME) \
  }                                                    \
  }                                                    \
  }                                                    \
  }                                                    \
  CAMP_HOST_TEST(CAMP_SUITE_NAME, CAMP_TEST_NAME)      \
  CAMP_CUDA_TEST(CAMP_SUITE_NAME, CAMP_TEST_NAME)      \
  CAMP_HIP_TEST(CAMP_SUITE_NAME, CAMP_TEST_NAME)

#endif  // CAMP_TEST_HPP
