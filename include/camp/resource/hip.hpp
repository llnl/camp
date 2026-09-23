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
#include <cstddef>
#include <mutex>
#include <utility>

#include "camp/defines.hpp"
#include "camp/helpers.hpp"
#include "camp/init_helpers.hpp"
#include "camp/resource/event.hpp"
#include "camp/resource/platform.hpp"

namespace camp
{
namespace resources
{
  inline namespace v1
  {
    class HipEvent;
    class Hip;

    template <>
    struct resource_from_platform<Platform::hip> {
      using type = ::camp::resources::Hip;
    };

    template <>
    struct is_concrete_event_impl<HipEvent> : std::true_type {
    };

    template <>
    struct is_concrete_resource_impl<Hip> : std::true_type {
    };

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
      explicit HipEvent(hipStream_t stream) : m_event(init(stream)) {}

      HipEvent(HipEvent const&) = delete;

      HipEvent(HipEvent&& rhs) noexcept
          : m_event(std::exchange(rhs.m_event, nullptr))
      {
      }

      HipEvent& operator=(HipEvent const&) = delete;

      HipEvent& operator=(HipEvent&& rhs) noexcept
      {
        finalize(m_event);
        m_event = std::exchange(rhs.m_event, nullptr);
        return *this;
      }

      ~HipEvent() { finalize(m_event); }

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
      friend inline bool operator==(HipEvent const& lhs,
                                    HipEvent const& rhs) = default;

      size_t get_hash() const
      {
        const size_t platform_type = size_t(get_platform()) << 32;
        size_t hash = std::hash<hipEvent_t>{}(m_event);
        return platform_type | (hash & 0xFFFFFFFF);
      }


    private:
      // note that hipEvent_t is an alias for a pointer and is nullable
      hipEvent_t m_event;

      static hipEvent_t init(hipStream_t stream)
      {
        hipEvent_t event;
        CAMP_HIP_API_INVOKE_AND_CHECK(hipEventCreateWithFlags,
                                      &event,
                                      hipEventDisableTiming);
        CAMP_HIP_API_INVOKE_AND_CHECK(hipEventRecord, event, stream);
        return event;
      }

      static void finalize(hipEvent_t& event)
      {
        if (event != nullptr) {
          CAMP_HIP_API_INVOKE_AND_CHECK(hipEventDestroy, event);
          event = nullptr;
        }
      }
    };

    struct HipStream
    {
      using handle_type = hipStream_t;

      explicit HipStream() : m_stream(init()) {}

      HipStream(HipStream const&) = delete;

      HipStream(HipStream&& rhs) noexcept
          : m_stream(std::exchange(rhs.m_stream, nullptr))
      {
      }

      HipStream& operator=(HipStream const&) = delete;

      HipStream& operator=(HipStream&& rhs) noexcept
      {
        finalize(m_stream);
        m_stream = std::exchange(rhs.m_stream, nullptr);
        return *this;
      }

      ~HipStream() { finalize(m_stream); }

      Platform get_platform() const { return Platform::hip; }

      void wait() const
      {
        CAMP_HIP_API_INVOKE_AND_CHECK(hipStreamSynchronize, m_stream);
      }

      handle_type get_handle() const { return m_stream; }

      /*
       * \brief Compares two events to see if they represent the same underlying
       *        hip stream.
       *
       * \return True if both refer to the same hip stream, false otherwise.
       */
      friend inline bool operator==(HipStream const& lhs,
                                    HipStream const& rhs) = default;

      size_t get_hash() const
      {
        const size_t platform_type = size_t(get_platform()) << 32;
        size_t hash = std::hash<hipStream_t>{}(m_stream);
        return platform_type | (hash & 0xFFFFFFFF);
      }


    private:
      // note that hipStream_t is an alias for a pointer and is nullable
      handle_type m_stream;

      static handle_type init()
      {
        handle_type stream;
        CAMP_HIP_API_INVOKE_AND_CHECK(hipStreamCreate, &stream);
        return stream;
      }

      static void finalize(handle_type& stream)
      {
        if (stream != nullptr) {
          CAMP_HIP_API_INVOKE_AND_CHECK(hipStreamDestroy, stream);
          stream = nullptr;
        }
      }
    };

    class Hip
    {
      static constexpr int num_streams = 16;

      template <typename T>
      using singleton_t = camp::optional_singleton<T, camp::OptionalDtorPolicy::None>;

      struct ExtraStream
      {
        std::array<HipStream, num_streams> streams;
        std::mutex lock;
        int previous{num_streams-1};
      };

      inline static constinit singleton_t<HipStream> default_stream;
      inline static constinit singleton_t<ExtraStream> extra_streams;

      static hipStream_t get_default_stream()
      {
#if !CAMP_USE_PLATFORM_DEFAULT_STREAM
        return default_stream.get_or_emplace().get_handle();
#else
        return nullptr;
#endif
      }

      static hipStream_t get_a_stream(int num)
      {
        auto& extra_state = extra_streams.get_or_emplace();

        if (num < 0) {
          std::lock_guard<std::mutex> lock(extra_state.lock);
          extra_state.previous = (extra_state.previous + 1) % num_streams;
          return extra_state.streams[extra_state.previous].get_handle();
        }

        return extra_state.streams[num % num_streams].get_handle();
      }

      // Private from-stream constructor
      Hip(hipStream_t s, int dev = 0) : stream(s), device(dev) {}

      MemoryAccess get_access_type(void* p)
      {
        hipPointerAttribute_t a;
        hipError_t status = hipPointerGetAttributes(&a, p);
        if (status == hipSuccess) {
          switch (a.type) {
            case hipMemoryTypeHost:
              return MemoryAccess::Pinned;
            case hipMemoryTypeDevice:
              return MemoryAccess::Device;
            case hipMemoryTypeManaged:
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
      using event_type = HipEvent;

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
        return Hip(get_default_stream());
      }

      /**
       * \brief Destroy all HIP streams created and managed by CAMP.
       *
       * Existing resources that refer to CAMP-managed streams are invalid
       * after this call. Streams passed to HipFromStream and the HIP platform
       * default stream are not destroyed. This function may be called
       * repeatedly, and later resource construction recreates the managed
       * streams.
       *
       * The caller must ensure no other thread is using HIP resources while
       * cleanup runs.
       */
      static void cleanup()
      {
        extra_streams.reset();
        default_stream.reset();
      }

      HipEvent get_event()
      {
        auto d{device_guard(get_device())};
        return HipEvent(get_stream());
      }

      Event get_event_erased() { return Event{get_event()}; }

      void wait()
      {
        auto d{device_guard(device)};
        CAMP_HIP_API_INVOKE_AND_CHECK(hipStreamSynchronize, stream);
      }

      void wait_for(HipEvent const& e)
      {
        auto d{device_guard(device)};
        CAMP_HIP_API_INVOKE_AND_CHECK(hipStreamWaitEvent,
                                      get_stream(),
                                      e.getHipEvent_t(),
                                      0);
      }

      void wait_for(Event const& e)
      {
        if (auto hip_event = e.try_get<HipEvent>()) {
          wait_for(*hip_event);
        } else {
          e.wait();
        }
      }

      // Memory
      template <typename T>
      T* allocate(size_t n, MemoryAccess ma = MemoryAccess::Device)
      {
        if (n == 0) {
          return nullptr;
        }
        T* ret = nullptr;
        auto d{device_guard(device)};
        switch (ma) {
          case MemoryAccess::Device:
            CAMP_HIP_API_INVOKE_AND_CHECK(hipMalloc,
                                          (void**)&ret,
                                          sizeof(T) * n);
            break;
          case MemoryAccess::Pinned:
            // TODO: do a test here for whether managed is *actually* shared
            // so we can use the better performing memory
            CAMP_HIP_API_INVOKE_AND_CHECK(hipHostMalloc,
                                          (void**)&ret,
                                          sizeof(T) * n);
            break;
          case MemoryAccess::Managed:
            CAMP_HIP_API_INVOKE_AND_CHECK(hipMallocManaged,
                                          (void**)&ret,
                                          sizeof(T) * n);
            break;
          case MemoryAccess::Unknown:
            ::camp::throw_re("Unknown memory access type, cannot allocate");
            break;
        }
        return ret;
      }

      void* calloc(size_t size, MemoryAccess ma)
      {
        if (size == 0) {
          return nullptr;
        }
        void* ret = allocate<char>(size, ma);
        if (ret != nullptr) {
          this->memset(ret, 0, size);
        }
        return ret;
      }

      void deallocate(void* p, MemoryAccess ma = MemoryAccess::Unknown)
      {
        if (p == nullptr) {
          return;
        }
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

      void memcpy(void* dst, const void* src, size_t size)
      {
        if (size == 0) {
          return;
        }
        auto d{device_guard(device)};
        CAMP_HIP_API_INVOKE_AND_CHECK(
            hipMemcpyAsync, dst, src, size, hipMemcpyDefault, stream);
      }

      void memset(void* p, int val, size_t size)
      {
        if (size == 0) {
          return;
        }
        auto d{device_guard(device)};
        CAMP_HIP_API_INVOKE_AND_CHECK(hipMemsetAsync, p, val, size, stream);
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
        size_t stream_hash = std::hash<void*>{}(static_cast<void*>(stream));
        return hip_type | (stream_hash & 0xFFFFFFFF);
      }

    private:
      hipStream_t stream;
      int device;
    };
  }  // namespace v1

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
  std::size_t operator()(const camp::resources::HipEvent& e) const
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
  std::size_t operator()(const camp::resources::Hip& h) const
  {
    return h.get_hash();
  }
};

}  // namespace std

#endif  // #ifdef CAMP_ENABLE_HIP

#endif /* __CAMP_HIP_HPP */
