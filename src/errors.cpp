//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include <camp/camp.hpp>
#include <exception>
#include <stdexcept>
#include <string>

namespace camp
{

void throw_re(const char* s) { throw std::runtime_error(s); }

void throw_re(std::string const& s) { throw std::runtime_error(s); }

#ifdef CAMP_ENABLE_CUDA

cudaError_t cudaAssert(cudaError_t code,
                       const char* call,
                       const char* file,
                       int line)
{
  if (code != cudaSuccess && code != cudaErrorNotReady) {
    std::string msg;
    msg += "campCudaErrchk(";
    msg += call;
    msg += ") ";
    msg += cudaGetErrorString(code);
    msg += " ";
    msg += file;
    msg += ":";
    msg += std::to_string(line);
    throw std::runtime_error(msg);
  }
  return code;
}

#endif  // #ifdef CAMP_ENABLE_CUDA

#ifdef CAMP_ENABLE_HIP

hipError_t hipAssert(hipError_t code,
                     const char* call,
                     const char* file,
                     int line)
{
  if (code != hipSuccess && code != hipErrorNotReady) {
    std::string msg;
    msg += "campHipErrchk(";
    msg += call;
    msg += ") ";
    msg += hipGetErrorString(code);
    msg += " ";
    msg += file;
    msg += ":";
    msg += std::to_string(line);
    throw std::runtime_error(msg);
  }
  return code;
}

#endif  // #ifdef CAMP_ENABLE_HIP

}  // namespace camp
