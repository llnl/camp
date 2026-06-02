//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef __CAMP_OMP_TARGET_HPP
#define __CAMP_OMP_TARGET_HPP

#include "camp/config.hpp"

#ifdef CAMP_ENABLE_TARGET_OPENMP

#include <omp.h>

#include <map>
#include <memory>
#include <mutex>

#include "camp/resource/event.hpp"
#include "camp/resource/platform.hpp"

namespace camp
{
namespace resources
{
  inline namespace v1
  {

    class OmpEvent
    {
    public:
      OmpEvent(char *addr_in, int device = omp_get_default_device())
          : addr(addr_in), dev(device)
      {
#pragma omp target device(dev) depend(inout : addr_in[0]) nowait
        {
        }
      }

      Platform get_platform() const { return Platform::omp_target; }

      bool check() const
      {
        // think up a way to do something better portably
        return false;
      }

      void wait() const
      {
        char *local_addr = addr;
        CAMP_ALLOW_UNUSED_LOCAL(local_addr);
        // if only we could use taskwait depend portably...
#pragma omp task if (0) depend(inout : local_addr[0])
        {
        }
      }

      void *getEventAddr() const { return addr; }

      /*
       * \brief Compares two events to see if they represent the same underlying
       *        openmp target event.
       *
       * \return True if both refer to the same address and device, false
       *         otherwise.
       */
      friend inline bool operator==(OmpEvent const& lhs, OmpEvent const& rhs) = default;

      size_t get_hash() const
      {
        const size_t platform_type = size_t(get_platform()) << 32;
        size_t hash = std::hash<char *>{}(addr);
        hash ^= std::hash<int>{}(dev) + 0x9E3779B9 + (hash << 6) + (hash >> 2);
        return platform_type | (hash & 0xFFFFFFFF);
      }

    private:
      char *addr;
      int dev;
    };

    class Omp
    {
      static char *get_addr(int num)
      {
        static constexpr int num_addrs = 16;
        static char s_addrs[num_addrs] = {};

        static std::mutex s_mtx;
        static int s_previous = num_addrs - 1;

        if (num < 0) {
          std::lock_guard<std::mutex> lock(s_mtx);
          s_previous = (s_previous + 1) % num_addrs;
          return &s_addrs[s_previous];
        }

        return &s_addrs[num % num_addrs];
      }

      // Private from-address constructor
      Omp(char *a, int device = omp_get_default_device()) : addr(a), dev(device)
      {
      }

      void check_ma(MemoryAccess ma)
      {
        if (ma != MemoryAccess::Device) {
          ::camp::throw_re(
              "OpenMP Target currently does not support allocating shared or "
              "managed memory");
        }
      }

    public:
      Omp(int group = -1, int device = omp_get_default_device())
          : addr(get_addr(group)), dev(device)
      {
      }

      /// Create a resource from a custom address
      /// The device specified must match the address, if none is specified the
      /// currently selected device is used.
      static Omp OmpFromAddr(char *a, int device = -1)
      {
        if (device < 0) {
          device = omp_get_default_device();
        }
        return Omp(a, device);
      }

      // Methods
      Platform get_platform() const { return Platform::omp_target; }

      static Omp get_default()
      {
        static Omp o;
        return o;
      }

      OmpEvent get_event() { return OmpEvent(addr, dev); }

      Event get_event_erased() { return Event{get_event()}; }

      void wait()
      {
        char *local_addr = addr;
        CAMP_ALLOW_UNUSED_LOCAL(local_addr);
#pragma omp target device(dev) depend(inout : local_addr[0])
        {
        }
      }

      void wait_for(Event *e)
      {
        OmpEvent *oe = e->try_get<OmpEvent>();
        if (oe) {
          char *local_addr = addr;
          char *other_addr = (char *)oe->getEventAddr();
          CAMP_ALLOW_UNUSED_LOCAL(local_addr);
          CAMP_ALLOW_UNUSED_LOCAL(other_addr);
#pragma omp target depend(inout : local_addr[0]) depend(in : other_addr[0]) \
    nowait
          {
          }
        } else {
          e->wait();
        }
      }

      // Memory
      template <typename T>
      T *allocate(size_t size, MemoryAccess ma = MemoryAccess::Device)
      {
        check_ma(ma);
        T *ret = static_cast<T *>(omp_target_alloc(sizeof(T) * size, dev));
        register_ptr_dev(ret, dev);
        return ret;
      }

      void *calloc(size_t size, MemoryAccess ma = MemoryAccess::Device)
      {
        check_ma(ma);
        void *p = allocate<char>(size);
        this->memset(p, 0, size);
        return p;
      }

      void deallocate(void *p, MemoryAccess ma = MemoryAccess::Device)
      {
        check_ma(ma);
#pragma omp critical(camp_register_ptr)
        {
          get_dev_register().erase(p);
        }
        omp_target_free(p, dev);
      }

      void memcpy(void *dst, const void *src, size_t size)
      {
        // this is truly, insanely awful, need to think of something better
        int dd = get_ptr_dev(dst);
        int sd = get_ptr_dev(src);
        // extra cast due to GCC openmp header bug
        omp_target_memcpy(dst, (void *)src, size, 0, 0, dd, sd);
      }

      void memset(void *p, int val, size_t size)
      {
        char *local_addr = addr;
        CAMP_ALLOW_UNUSED_LOCAL(local_addr);
        char *pc = (char *)p;
#pragma omp target teams distribute parallel for device(dev) \
    depend(inout : local_addr[0]) is_device_ptr(pc) nowait
        for (size_t i = 0; i < size; ++i) {
          pc[i] = val;
        }
      }

      void register_ptr_dev(void *p, int device)
      {
#pragma omp critical(camp_register_ptr)
        {
          get_dev_register()[p] = device;
        }
      }

      int get_ptr_dev(void const *p)
      {
        int ret = omp_get_initial_device();
#pragma omp critical(camp_register_ptr)
        {
          auto it = get_dev_register().find(p);
          if (it != get_dev_register().end()) {
            ret = it->second;
          }
        }
        return ret;
      }

      /*
       * \brief Get the device associated with this Omp resource.
       *
       * \code
       * camp::resources::Omp res;
       * #pragma omp target device(res.get_device())
       * \endcode
       *
       * \return The device id of this Omp resource.
       */
      int get_device() const { return dev; }

      /*
       * \brief Get the depend address associated with this Omp resource.
       *        This address is used in depend clauses with tasks or nowait.
       *
       * \code
       * camp::resources::Omp res;
       * #pragma omp target depend(inout : res.get_depend_location()[0]) nowait
       * \endcode
       *
       * \return The depend address of this Omp resource.
       */
      char *get_depend_location() const { return addr; }

      /*
       * \brief Compares two (Omp) resources to see if they are equal
       *
       * \return True or false depending on if this is the same dev and addr ptr
       */
      friend inline bool operator==(Omp const& lhs, Omp const& rhs) = default;

      size_t get_hash() const
      {
        const size_t omp_type = size_t(get_platform()) << 32;
        size_t stream_hash = std::hash<void *>{}(static_cast<void *>(addr));
        return omp_type | (stream_hash & 0xFFFFFFFF);
      }

    private:
      char *addr;
      int dev;

      template <typename always_void_odr_helper = void>
      std::map<const void *, int> &get_dev_register()
      {
        static std::map<const void *, int> dev_register;
        return dev_register;
      }
    };

  }  // namespace v1

  template <>
  struct is_concrete_resource_impl<Omp> : std::true_type {
  };
}  // namespace resources
}  // namespace camp

namespace std
{

/*
 * \brief Specialization of std::hash for camp::resources::OmpEvent
 *
 * Provides a hash function for omp typed event objects, enabling their use
 * as keys in unordered associative containers (std::unordered_map,
 * std::unordered_set, etc.)
 *
 * \return A size_t hash value
 */
template <>
struct hash<camp::resources::OmpEvent> {
  std::size_t operator()(const camp::resources::OmpEvent& e) const
  {
    return e.get_hash();
  }
};

/*
 * \brief Specialization of std::hash for camp::resources::Omp
 *
 * Provides a hash function for Omp typed resource objects, enabling their use
 * as keys in unordered associative containers (std::unordered_map,
 * std::unordered_set, etc.)
 *
 * \return A size_t hash value
 */
template <>
struct hash<camp::resources::Omp> {
  std::size_t operator()(const camp::resources::Omp &o) const
  {
    return o.get_hash();
  }
};

}  // namespace std
#endif  // #ifdef CAMP_ENABLE_TARGET_OPENMP

#endif /* __CAMP_OMP_TARGET_HPP */
