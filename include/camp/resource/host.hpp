//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef __CAMP_HOST_HPP
#define __CAMP_HOST_HPP

#include <cstdlib>
#include <cstring>

#include "camp/resource/event.hpp"
#include "camp/resource/platform.hpp"

namespace camp
{
namespace resources
{
  inline namespace v1
  {

    class HostEvent
    {
    public:
      HostEvent() {}

      Platform get_platform() const { return Platform::host; }

      bool check() const { return true; }

      void wait() const {}

      /*
       * \brief Compares two events to see if they are equal
       *
       * \return True or false depending on if this is the same event
       */
      friend inline bool operator==(HostEvent const& lhs, HostEvent const& rhs) = default;

      size_t get_hash() const
      {
        const size_t platform_type = size_t(get_platform()) << 32;
        return platform_type;
      }
    };

    class Host
    {
    public:
      Host(int /* group */ = -1) {}

      // Methods
      Platform get_platform() const { return Platform::host; }

      static Host get_default()
      {
        static Host h;
        return h;
      }

      HostEvent get_event() { return HostEvent(); }

      Event get_event_erased()
      {
        Event e{HostEvent()};
        return e;
      }

      void wait() {}

      void wait_for(Event *e) { e->wait(); }

      // Memory
      template <typename T>
      T *allocate(size_t n, MemoryAccess = MemoryAccess::Device)
      {
        return (T *)std::malloc(sizeof(T) * n);
      }

      void *calloc(size_t size, MemoryAccess = MemoryAccess::Device)
      {
        void *p = allocate<char>(size);
        this->memset(p, 0, size);
        return p;
      }

      void deallocate(void *p, MemoryAccess = MemoryAccess::Device)
      {
        std::free(p);
      }

      void memcpy(void *dst, const void *src, size_t size)
      {
        std::memcpy(dst, src, size);
      }

      void memset(void *p, int val, size_t size) { std::memset(p, val, size); }

      /*
       * \brief Compares two (Host) resources to see if they are equal
       *
       * \return Always return true since Host resources are always the same
       */
      friend inline bool operator==(Host const& CAMP_UNUSED_ARG(lhs), Host const& CAMP_UNUSED_ARG(rhs))
      {
        return true;
      }

      size_t get_hash() const
      {
        return size_t(get_platform()) << 32;  // All Host resources are the same
      }
    };

  }  // namespace v1

  template <>
  struct is_concrete_resource_impl<Host> : std::true_type {
  };

}  // namespace resources
}  // namespace camp

namespace std
{

/*
 * \brief Specialization of std::hash for camp::resources::HostEvent
 *
 * Provides a hash function for host typed event objects, enabling their use
 * as keys in unordered associative containers (std::unordered_map,
 * std::unordered_set, etc.)
 *
 * \return Hash value for the host (always the value of get_platform())
 */
template <>
struct hash<camp::resources::HostEvent> {
  std::size_t operator()(const camp::resources::HostEvent &e) const
  {
    return e.get_hash();
  }
};

/*
 * \brief Specialization of std::hash for camp::resources::Host
 *
 * Provides a hash function for Host typed resource objects, enabling their use
 * as keys in unordered associative containers (std::unordered_map,
 * std::unordered_set, etc.)
 *
 * \return Hash value for the host (always the value of get_platform())
 */
template <>
struct hash<camp::resources::Host> {
  std::size_t operator()(const camp::resources::Host &h) const
  {
    return h.get_hash();
  }
};

}  // namespace std
#endif /* __CAMP_DEVICES_HPP */
