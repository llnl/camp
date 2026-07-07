//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef CAMP_DEFINES_HPP
#define CAMP_DEFINES_HPP

#include <camp/config.hpp>
#include <cstddef>
#include <cstdint>
#include <string>

// include cuda header if configured, even if not in use
#ifdef CAMP_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

#ifdef CAMP_ENABLE_HIP
#include <hip/hip_runtime.h>
#endif

namespace camp
{

#define CAMP_UNUSED_ARG(X)

#define CAMP_ALLOW_UNUSED_LOCAL(X) (void)(X)

#define CAMP_STRINGIFY_HELPER(...) #__VA_ARGS__
#define CAMP_STRINGIFY(...) CAMP_STRINGIFY_HELPER(__VA_ARGS__)

#if defined(__clang__)
#define CAMP_COMPILER_CLANG
#elif defined(__INTEL_COMPILER)
#define CAMP_COMPILER_INTEL
#elif defined(__xlc__)
#define CAMP_COMPILER_XLC
#elif defined(__PGI)
#define CAMP_COMPILER_PGI
#elif defined(_WIN32)
#define CAMP_COMPILER_MSVC
#elif defined(__GNUC__)
#define CAMP_COMPILER_GNU
#else
#pragma warn("Unknown compiler!")
#endif

// detect empty_bases for MSVC
#ifndef __has_declspec_attribute
#define __has_declspec_attribute(__x) 0
#endif
#if defined(CAMP_COMPILER_MSVC) || __has_declspec_attribute(empty_bases)
#define CAMP_EMPTY_BASES __declspec(empty_bases)
#else
#define CAMP_EMPTY_BASES
#endif

// define host device macros
#define CAMP_HIP_HOST_DEVICE

#if defined(CAMP_ENABLE_CUDA) && defined(__CUDACC__)
#define CAMP_DEVICE __device__
#define CAMP_HOST_DEVICE __host__ __device__
#define CAMP_HAVE_CUDA 1

#if defined(__NVCC__)
#if defined(_WIN32)  // windows is non-compliant, yay
#define CAMP_SUPPRESS_HD_WARN __pragma(nv_exec_check_disable)
#else
#define CAMP_SUPPRESS_HD_WARN _Pragma("nv_exec_check_disable")
#endif
#else  // only nvcc supports this pragma
#define CAMP_SUPPRESS_HD_WARN
#endif

#elif defined(CAMP_ENABLE_HIP) && defined(__HIPCC__)
#define CAMP_DEVICE __device__
#define CAMP_HOST_DEVICE __host__ __device__
#define CAMP_HAVE_HIP 1
#undef CAMP_HIP_HOST_DEVICE
#define CAMP_HIP_HOST_DEVICE __host__ __device__
#define CAMP_HAVE_HIP 1

#define CAMP_SUPPRESS_HD_WARN

#elif defined(CAMP_ENABLE_SYCL) && defined(SYCL_LANGUAGE_VERSION)
#define CAMP_HAVE_SYCL 1
#define CAMP_DEVICE
#define CAMP_HOST_DEVICE
#define CAMP_SUPPRESS_HD_WARN

#else
#define CAMP_DEVICE
#define CAMP_HOST_DEVICE
#define CAMP_SUPPRESS_HD_WARN
#endif

#if defined(CAMP_ENABLE_OPENMP) && defined(_OPENMP)
#define CAMP_HAVE_OPENMP 1
#endif

#if defined(CAMP_ENABLE_TARGET_OPENMP)
#if _OPENMP >= 201511
#define CAMP_HAVE_OMP_OFFLOAD 1
#else
#define CAMP_HAVE_OMP_OFFLOAD 0
#warning Compiler does NOT support OpenMP Target Offload even though user has enabled it!
#endif
#endif

// Compiler checks
#if defined(__NVCC__) && __CUDACC_VER_MAJOR__ < 10
#error nvcc below 10 is not supported
#endif
// This works for:
//   clang
//   nvcc 10 and higher using clang as a host compiler
//   MSVC 1911... and higher, see check below
//   XL C++ at least back to 16.1.0, possibly farther
#define CAMP_USE_MAKE_INTEGER_SEQ 0
#define CAMP_USE_TYPE_PACK_ELEMENT 0

#if defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 191125507
// __has_builtin exists but does not always expose this
#undef CAMP_USE_MAKE_INTEGER_SEQ
#define CAMP_USE_MAKE_INTEGER_SEQ 1
// __type_pack_element remains unsupported
#elif defined(__has_builtin)
#if __has_builtin(__make_integer_seq)
#undef CAMP_USE_MAKE_INTEGER_SEQ
#define CAMP_USE_MAKE_INTEGER_SEQ 1
#undef CAMP_USE_TYPE_PACK_ELEMENT
#define CAMP_USE_TYPE_PACK_ELEMENT 1
#endif
#endif

// This works for:
//   GCC >= 8
//   intel 19+ in GCC 8 or higher mode
//   nvcc 10+ in GCC 8 or higher mode, no lower nvcc allowed anyway
//   PGI 19+ in GCC 8 or higher mode
#if __GNUC__ >= 8                                               \
    && (/* intel compiler in gcc 8+ mode */                     \
        ((!defined(__INTEL_COMPILER))                           \
         || __INTEL_COMPILER >= 1900) /* nvcc in gcc 8+ mode */ \
        || ((!defined(__PGIC__)) || __PGIC__ >= 19))
#define CAMP_USE_INTEGER_PACK 1
#else
#define CAMP_USE_INTEGER_PACK 0
#endif

// libstdc++ from GCC below version 5 lacks the type trait
#if defined(__GLIBCXX__) && (__GLIBCXX__ < 20150422 || __GNUC__ < 5)
#define CAMP_HAS_IS_TRIVIALLY_COPY_CONSTRUCTIBLE 0
#else
#define CAMP_HAS_IS_TRIVIALLY_COPY_CONSTRUCTIBLE 1
#endif

// Types
using idx_t = std::ptrdiff_t;
using nullptr_t = decltype(nullptr);

// Helper macros
// TODO: -> CAMP_MAKE_LAMBDA_CONSUMER
#define CAMP_MAKE_L(X)                                             \
  template <typename Lambda, typename... Rest>                     \
  struct X##_l {                                                   \
    using type = typename X<Lambda::template expr, Rest...>::type; \
  }


/// Throw a runtime_error, avoid including exception everywhere
[[noreturn]]
CAMP_DLL_EXPORT void throw_re(const char *s);
///
[[noreturn]]
CAMP_DLL_EXPORT void throw_re(std::string const &s);

#ifdef CAMP_ENABLE_CUDA

#define campCudaErrchk(ans) ::camp::cudaAssert((ans), #ans, __FILE__, __LINE__)

#define campCudaErrchkDiscardReturn(ans) \
  (void)::camp::cudaAssert((ans), #ans, __FILE__, __LINE__)

[[deprecated]]
CAMP_DLL_EXPORT cudaError_t
cudaAssert(cudaError_t code, const char *call, const char *file, int line);

/// Invoke the given CUDA API function and report CUDA error codes.
///
/// NOTE: This prints arguments to the given function so the arguments
///       are evaluated twice on an error.
#define CAMP_CUDA_API_INVOKE_AND_CHECK_IMPLEMENTATION(func, ...)          \
  /* Avoid shadowing by adding 56792578 to variable names */              \
  cudaError_t code_56792578 = func(__VA_ARGS__);                          \
  if (code_56792578 != cudaSuccess                                        \
      && code_56792578 != cudaErrorNotReady) /* [[unlikely]] */           \
  {                                                                       \
    static constexpr auto func_name_56792578 = CAMP_STRINGIFY(func);      \
    static constexpr auto arg_names_56792578 =                            \
        ::camp::experimental::cuda_get_api_arg_names(func_name_56792578); \
    ::camp::reportError("CUDA",                                           \
                        cudaGetErrorString(code_56792578),                \
                        func_name_56792578,                               \
                        arg_names_56792578,                               \
                        std::forward_as_tuple(__VA_ARGS__),               \
                        __FILE__,                                         \
                        __LINE__);                                        \
  }
///
#define CAMP_CUDA_API_INVOKE_AND_CHECK_RETURN(...)             \
  [&]() {                                                      \
    CAMP_CUDA_API_INVOKE_AND_CHECK_IMPLEMENTATION(__VA_ARGS__) \
    return code_56792578;                                      \
  }()
///
#define CAMP_CUDA_API_INVOKE_AND_CHECK(...)                    \
  do {                                                         \
    CAMP_CUDA_API_INVOKE_AND_CHECK_IMPLEMENTATION(__VA_ARGS__) \
  } while (false)

#endif  // #ifdef CAMP_ENABLE_CUDA


#ifdef CAMP_ENABLE_HIP

#define campHipErrchk(ans) ::camp::hipAssert((ans), #ans, __FILE__, __LINE__)

#define campHipErrchkDiscardReturn(ans) \
  (void)::camp::hipAssert((ans), #ans, __FILE__, __LINE__)

[[deprecated]]
CAMP_DLL_EXPORT hipError_t
hipAssert(hipError_t code, const char *call, const char *file, int line);


/// Invoke the given HIP API function and report HIP error codes.
///
/// NOTE: This prints arguments to the given function so the arguments
///       are evaluated twice on an error.
#define CAMP_HIP_API_INVOKE_AND_CHECK_IMPLEMENTATION(func, ...)          \
  /* Avoid shadowing by adding 56792578 to variable names */             \
  hipError_t code_56792578 = func(__VA_ARGS__);                          \
  if (code_56792578 != hipSuccess                                        \
      && code_56792578 != hipErrorNotReady) /* [[unlikely]] */           \
  {                                                                      \
    static constexpr auto func_name_56792578 = CAMP_STRINGIFY(func);     \
    static constexpr auto arg_names_56792578 =                           \
        ::camp::experimental::hip_get_api_arg_names(func_name_56792578); \
    ::camp::reportError("HIP",                                           \
                        hipGetErrorString(code_56792578),                \
                        func_name_56792578,                              \
                        arg_names_56792578,                              \
                        std::forward_as_tuple(__VA_ARGS__),              \
                        __FILE__,                                        \
                        __LINE__);                                       \
  }
///
#define CAMP_HIP_API_INVOKE_AND_CHECK_RETURN(...)             \
  [&]() {                                                     \
    CAMP_HIP_API_INVOKE_AND_CHECK_IMPLEMENTATION(__VA_ARGS__) \
    return code_56792578;                                     \
  }()
///
#define CAMP_HIP_API_INVOKE_AND_CHECK(...)                    \
  do {                                                        \
    CAMP_HIP_API_INVOKE_AND_CHECK_IMPLEMENTATION(__VA_ARGS__) \
  } while (false)

#endif  // #ifdef CAMP_ENABLE_HIP

}  // namespace camp

#endif  // CAMP_DEFINES_HPP
