//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef __CAMP_HIP_HPP
#define __CAMP_HIP_HPP

#include "camp/config.hpp"

#ifdef CAMP_ENABLE_HIP

#include <hip/hip_runtime.h>

#include <array>
#include <mutex>

#include "camp/defines.hpp"
#include "camp/helpers.hpp"
#include "camp/resource/event.hpp"
#include "camp/resource/platform.hpp"

namespace camp
{
namespace resources
{
  inline namespace v1
  {
    class Hip;

    namespace
    {
      struct device_guard {
        device_guard(int device)
        {
          CAMP_HIP_API_INVOKE_AND_CHECK(hipGetDevice, &prev_device);
          if (device != prev_device) {
            CAMP_HIP_API_INVOKE_AND_CHECK(hipSetDevice, device);
          } else {
            prev_device = -1;
          }
        }

        ~device_guard()
        {
          if (prev_device != -1) {
            CAMP_HIP_API_INVOKE_AND_CHECK(hipSetDevice, prev_device);
          }
        }

        int prev_device = -1;
      };

    }  // namespace

    class HipEvent
    {
    public:
      HipEvent(hipStream_t stream) { init(stream); }

      HipEvent(Hip &res);

      Platform get_platform() const { return Platform::hip; }

      bool check() const
      {
        return (CAMP_HIP_API_INVOKE_AND_CHECK_RETURN(hipEventQuery, m_event)
                == hipSuccess);
      }

      void wait() const
      {
        CAMP_HIP_API_INVOKE_AND_CHECK(hipEventSynchronize, m_event);
      }

      hipEvent_t getHipEvent_t() const { return m_event; }

      /*
       * \brief Compares two events to see if they represent the same underlying
       *        hip event.
       *
       * \return True if both refer to the same hip event, false otherwise.
       */
      friend inline bool operator==(HipEvent const& lhs, HipEvent const& rhs) = default;

      size_t get_hash() const
      {
        const size_t platform_type = size_t(get_platform()) << 32;
        size_t hash = std::hash<hipEvent_t>{}(m_event);
        return platform_type | (hash & 0xFFFFFFFF);
      }

    private:
      // note that cudaEvent_t is an alias for a pointer and is nullable
      hipEvent_t m_event;

      void init(hipStream_t stream)
      {
        CAMP_HIP_API_INVOKE_AND_CHECK(hipEventCreateWithFlags,
                                      &m_event,
                                      hipEventDisableTiming);
        CAMP_HIP_API_INVOKE_AND_CHECK(hipEventRecord, m_event, stream);
      }
    };

    class Hip
    {
      static hipStream_t get_a_stream(int num)
      {
        static constexpr int num_streams = 16;
        static std::array<hipStream_t, num_streams> s_streams = [] {
          std::array<hipStream_t, num_streams> streams;
          for (auto &s : streams) {
            CAMP_HIP_API_INVOKE_AND_CHECK(hipStreamCreate, &s);
          }
          return streams;
        }();

        static std::mutex s_mtx;
        static int s_previous = num_streams - 1;

        if (num < 0) {
          std::lock_guard<std::mutex> lock(s_mtx);
          s_previous = (s_previous + 1) % num_streams;
          return s_streams[s_previous];
        }

        return s_streams[num % num_streams];
      }

      // Private from-stream constructor
      Hip(hipStream_t s, int dev = 0) : stream(s), device(dev) {}

      MemoryAccess get_access_type(void *p)
      {
        hipPointerAttribute_t a;
        hipError_t status = hipPointerGetAttributes(&a, p);
        if (status == hipSuccess) {
#if (HIP_VERSION_MAJOR >= 6)
          switch (a.type) {
#else
          switch (a.memoryType) {
#endif
            case hipMemoryTypeHost:
              return MemoryAccess::Pinned;
            case hipMemoryTypeDevice:
              return MemoryAccess::Device;
            case hipMemoryTypeUnified:
              return MemoryAccess::Managed;
            default:
              return MemoryAccess::Unknown;
          }
        }
        ::camp::throw_re("invalid pointer detected");
        // unreachable
        return MemoryAccess::Unknown;
      }

    public:
      Hip(int group = -1, int dev = 0)
          : stream(get_a_stream(group)), device(dev)
      {
      }

      /// Create a resource from a custom stream
      /// The device specified must match the stream, if none is specified the
      /// currently selected device is used.
      static Hip HipFromStream(hipStream_t s, int dev = -1)
      {
        if (dev < 0) {
          CAMP_HIP_API_INVOKE_AND_CHECK(hipGetDevice, &dev);
        }
        return Hip(s, dev);
      }

      // Methods
      Platform get_platform() const { return Platform::hip; }

      static Hip get_default()
      {
        static Hip h([] {
          hipStream_t s;
#if CAMP_USE_PLATFORM_DEFAULT_STREAM
          s = 0;
#else
          CAMP_HIP_API_INVOKE_AND_CHECK(hipStreamCreate, &s);
#endif
          return s;
        }());
        return h;
      }

      HipEvent get_event() { return HipEvent(*this); }

      Event get_event_erased() { return Event{HipEvent(*this)}; }

      void wait()
      {
        auto d{device_guard(device)};
        CAMP_HIP_API_INVOKE_AND_CHECK(hipStreamSynchronize, stream);
      }

      void wait_for(Event *e)
      {
        auto *hip_event = e->try_get<HipEvent>();
        if (hip_event) {
          auto d{device_guard(device)};
          CAMP_HIP_API_INVOKE_AND_CHECK(hipStreamWaitEvent,
                                        get_stream(),
                                        hip_event->getHipEvent_t(),
                                        0);
        } else {
          e->wait();
        }
      }

      // Memory
      template <typename T>
      T *allocate(size_t size, MemoryAccess ma = MemoryAccess::Device)
      {
        T *ret = nullptr;
        if (size > 0) {
          auto d{device_guard(device)};
          switch (ma) {
            case MemoryAccess::Unknown:
            case MemoryAccess::Device:
              CAMP_HIP_API_INVOKE_AND_CHECK(hipMalloc,
                                            (void **)&ret,
                                            sizeof(T) * size);
              break;
            case MemoryAccess::Pinned:
              // TODO: do a test here for whether managed is *actually* shared
              // so we can use the better performing memory
              CAMP_HIP_API_INVOKE_AND_CHECK(hipHostMalloc,
                                            (void **)&ret,
                                            sizeof(T) * size);
              break;
            case MemoryAccess::Managed:
              CAMP_HIP_API_INVOKE_AND_CHECK(hipMallocManaged,
                                            (void **)&ret,
                                            sizeof(T) * size);
              break;
          }
        }
        return ret;
      }

      void *calloc(size_t size, MemoryAccess ma)
      {
        void *p = allocate<char>(size, ma);
        this->memset(p, 0, size);
        return p;
      }

      void deallocate(void *p, MemoryAccess ma = MemoryAccess::Unknown)
      {
        auto d{device_guard(device)};
        if (ma == MemoryAccess::Unknown) {
          ma = get_access_type(p);
        }
        switch (ma) {
          case MemoryAccess::Device:
            CAMP_HIP_API_INVOKE_AND_CHECK(hipFree, p);
            break;
          case MemoryAccess::Pinned:
            // TODO: do a test here for whether managed is *actually* shared
            // so we can use the better performing memory
            CAMP_HIP_API_INVOKE_AND_CHECK(hipHostFree, p);
            break;
          case MemoryAccess::Managed:
            CAMP_HIP_API_INVOKE_AND_CHECK(hipFree, p);
            break;
          case MemoryAccess::Unknown:
            ::camp::throw_re("Unknown memory access type, cannot free");
            break;
        }
      }

      void memcpy(void *dst, const void *src, size_t size)
      {
        if (size > 0) {
          auto d{device_guard(device)};
          CAMP_HIP_API_INVOKE_AND_CHECK(
              hipMemcpyAsync, dst, src, size, hipMemcpyDefault, stream);
        }
      }

      void memset(void *p, int val, size_t size)
      {
        if (size > 0) {
          auto d{device_guard(device)};
          CAMP_HIP_API_INVOKE_AND_CHECK(hipMemsetAsync, p, val, size, stream);
        }
      }

      hipStream_t get_stream() const { return stream; }

      int get_device() const { return device; }

      /*
       * \brief Compares two (Hip) resources to see if they are equal
       *
       * \return True or false depending on if this is the same stream
       */
      friend inline bool operator==(Hip const& lhs, Hip const& rhs) = default;

      size_t get_hash() const
      {
        const size_t hip_type = size_t(get_platform()) << 32;
        size_t stream_hash = std::hash<void *>{}(static_cast<void *>(stream));
        return hip_type | (stream_hash & 0xFFFFFFFF);
      }

    private:
      hipStream_t stream;
      int device;
    };

    inline HipEvent::HipEvent(Hip &res)
    {
      auto d{device_guard(res.get_device())};
      init(res.get_stream());
    }

  }  // namespace v1

  template <>
  struct is_concrete_resource_impl<Hip> : std::true_type {
  };
}  // namespace resources
}  // namespace camp

namespace std
{

/*
 * \brief Specialization of std::hash for camp::resources::HipEvent
 *
 * Provides a hash function for hip typed event objects, enabling their use
 * as keys in unordered associative containers (std::unordered_map,
 * std::unordered_set, etc.)
 *
 * \return A size_t hash value
 */
template <>
struct hash<camp::resources::HipEvent> {
  std::size_t operator()(const camp::resources::HipEvent &e) const
  {
    return e.get_hash();
  }
};

/*
 * \brief Specialization of std::hash for camp::resources::Hip
 *
 * Provides a hash function for hip typed resource objects, enabling their use
 * as keys in unordered associative containers (std::unordered_map,
 * std::unordered_set, etc.)
 *
 * \return A size_t hash value
 */
template <>
struct hash<camp::resources::Hip> {
  std::size_t operator()(const camp::resources::Hip &h) const
  {
    return h.get_hash();
  }
};

}  // namespace std

#endif  // #ifdef CAMP_ENABLE_HIP

#endif /* __CAMP_HIP_HPP */
