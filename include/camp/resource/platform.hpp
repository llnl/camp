//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef __CAMP_PLATFORM_HPP
#define __CAMP_PLATFORM_HPP

namespace camp
{
namespace resources
{
  inline namespace v1
  {

    enum class Platform {
      undefined = 0,
      host = 1,
      cuda = 2,
      omp_target = 4,
      hip = 8,
      sycl = 16
    };

    enum class MemoryAccess { Unknown, Device, Pinned, Managed };

    template <Platform p>
    struct resource_from_platform;

  }  // namespace v1
}  // namespace resources
}  // namespace camp
#endif /* __CAMP_PLATFORM_HPP */
