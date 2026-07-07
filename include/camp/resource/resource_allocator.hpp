//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef __CAMP_RESOURCE_ALLOCATOR_HPP
#define __CAMP_RESOURCE_ALLOCATOR_HPP

#include <cstddef>

#include "camp/config.hpp"
#include "camp/resource/platform.hpp"

namespace camp
{
namespace resources
{
template<typename Resource>
struct ResourceAllocator
{
  template<typename T>
  struct allocator
  {
    using value_type = T;

    allocator(Resource res = Resource {},
              camp::resources::MemoryAccess mem_type =
                  camp::resources::MemoryAccess::Device)
        : m_res {res},
          m_mem_type {mem_type}
    {}

    allocator(allocator const&) = default;
    allocator(allocator&&)      = default;

    allocator& operator=(allocator const&) = default;
    allocator& operator=(allocator&&)      = default;

    template<typename U>
    allocator(allocator<U> const& other) noexcept
        : m_res {other.get_resource()},
          m_mem_type {other.get_mem_access()}
    {}

    [[nodiscard]]
    value_type* allocate(std::size_t num)
    {
      if (num > std::numeric_limits<std::size_t>::max() / sizeof(value_type))
      {
        throw std::bad_alloc();
      }

      value_type* ptr = m_res.template allocate<value_type>(num, m_mem_type);

      if (!ptr)
      {
        throw std::bad_alloc();
      }

      return ptr;
    }

    void deallocate(value_type* ptr, std::size_t) noexcept
    {
      m_res.deallocate(ptr, m_mem_type);
    }

    [[nodiscard]]
    Resource const& get_resource() const noexcept { return m_res; }

    [[nodiscard]]
    Resource get_resource() noexcept { return m_res; }

    [[nodiscard]]
    camp::resources::MemoryAccess get_mem_access() const noexcept
    {
      return m_mem_type;
    }

    template<typename U>
    friend inline bool operator==(allocator const& lhs, allocator<U> const& rhs)
    {
      return lhs.get_resource() == rhs.get_resource() &&
             lhs.get_mem_access() == rhs.get_mem_access();
    }

  private:
    Resource m_res;
    camp::resources::MemoryAccess m_mem_type;
  };
};
}  // namespace detail

template<typename T, typename Resource>
using ResourceAllocator =
    typename resources::ResourceAllocator<Resource>::template allocator<T>;

}  // namespace camp 

#endif  // __CAMP_RESOURCE_ALLOCATOR_HPP
