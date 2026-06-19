//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef __CAMP_CUDA_HPP
#define __CAMP_CUDA_HPP

#include "camp/config.hpp"

#ifdef CAMP_ENABLE_CUDA

#include <cuda_runtime.h>

#include <cstddef>
#include <array>
#include <mutex>
#include <utility>

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
    class CudaEvent;
    class Cuda;

    template <>
    struct resource_from_platform<Platform::cuda> {
      using type = ::camp::resources::Cuda;
    };

    template <>
    struct is_concrete_event_impl<CudaEvent> : std::true_type {
    };

    template <>
    struct is_concrete_resource_impl<Cuda> : std::true_type {
    };

    namespace
    {
      struct device_guard {
        device_guard(int device)
        {
          CAMP_CUDA_API_INVOKE_AND_CHECK(cudaGetDevice, &prev_device);
          if (device != prev_device) {
            CAMP_CUDA_API_INVOKE_AND_CHECK(cudaSetDevice, device);
          } else {
            prev_device = -1;
          }
        }

        ~device_guard()
        {
          if (prev_device != -1) {
            CAMP_CUDA_API_INVOKE_AND_CHECK(cudaSetDevice, prev_device);
          }
        }

        int prev_device = -1;
      };

    }  // namespace

    class CudaEvent
    {
    public:
      explicit CudaEvent(cudaStream_t stream)
        : m_event(init(stream))
      {}

      CudaEvent(CudaEvent const&) = delete;

      CudaEvent(CudaEvent&& rhs) noexcept
        : m_event(std::exchange(rhs.m_event, nullptr))
      {}

      CudaEvent& operator=(CudaEvent const&) = delete;

      CudaEvent& operator=(CudaEvent&& rhs) noexcept
      {
        finalize(m_event);
        m_event = std::exchange(rhs.m_event, nullptr);
        return *this;
      }

      ~CudaEvent()
      {
        finalize(m_event);
      }

      Platform get_platform() const { return Platform::cuda; }

      bool check() const
      {
        return (CAMP_CUDA_API_INVOKE_AND_CHECK_RETURN(cudaEventQuery, m_event)
                == cudaSuccess);
      }

      void wait() const
      {
        CAMP_CUDA_API_INVOKE_AND_CHECK(cudaEventSynchronize, m_event);
      }

      cudaEvent_t getCudaEvent_t() const { return m_event; }

      /*
       * \brief Compares two events to see if they represent the same underlying
       *        cuda event.
       *
       * \return True if both refer to the same cuda event, false otherwise.
       */
      friend inline bool operator==(CudaEvent const& lhs, CudaEvent const& rhs) = default;

      size_t get_hash() const
      {
        const size_t platform_type = size_t(get_platform()) << 32;
        size_t hash = std::hash<cudaEvent_t>{}(m_event);
        return platform_type | (hash & 0xFFFFFFFF);
      }

    private:
      // note that cudaEvent_t is an alias for a pointer and is nullable
      cudaEvent_t m_event;

      static cudaEvent_t init(cudaStream_t stream)
      {
        cudaEvent_t event;
        CAMP_CUDA_API_INVOKE_AND_CHECK(cudaEventCreateWithFlags,
                                       &event,
                                       cudaEventDisableTiming);
        CAMP_CUDA_API_INVOKE_AND_CHECK(cudaEventRecord, event, stream);
        return event;
      }

      static void finalize(cudaEvent_t& event)
      {
        if (event != nullptr) {
          CAMP_CUDA_API_INVOKE_AND_CHECK(cudaEventDestroy, event);
          event = nullptr;
        }
      }
    };

    class Cuda
    {
      static cudaStream_t get_a_stream(int num)
      {
        static constexpr int num_streams = 16;
        static std::array<cudaStream_t, num_streams> s_streams = [] {
          std::array<cudaStream_t, num_streams> streams;
          for (auto &s : streams) {
            CAMP_CUDA_API_INVOKE_AND_CHECK(cudaStreamCreate, &s);
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
      Cuda(cudaStream_t s, int dev = 0) : stream(s), device(dev) {}

      MemoryAccess get_access_type(void *p)
      {
        cudaPointerAttributes a;
        cudaError_t status = cudaPointerGetAttributes(&a, p);
        if (status == cudaSuccess) {
          switch (a.type) {
            case cudaMemoryTypeUnregistered:
              return MemoryAccess::Unknown;
            case cudaMemoryTypeHost:
              return MemoryAccess::Pinned;
            case cudaMemoryTypeDevice:
              return MemoryAccess::Device;
            case cudaMemoryTypeManaged:
              return MemoryAccess::Managed;
          }
        }
        ::camp::throw_re("invalid pointer detected");
        // This return statement exists because compilers do not determine the
        // above unconditionally throws
        // related:
        // https://stackoverflow.com/questions/64523302/cuda-missing-return-statement-at-end-of-non-void-function-in-constexpr-if-fun
        return MemoryAccess::Unknown;
      }

    public:
      using event_type = CudaEvent;

      Cuda(int group = -1, int dev = 0)
          : stream(get_a_stream(group)), device(dev)
      {
      }

      /// Create a resource from a custom stream
      /// The device specified must match the stream, if none is specified the
      /// currently selected device is used.
      static Cuda CudaFromStream(cudaStream_t s, int dev = -1)
      {
        if (dev < 0) {
          CAMP_CUDA_API_INVOKE_AND_CHECK(cudaGetDevice, &dev);
        }
        return Cuda(s, dev);
      }

      // Methods
      Platform get_platform() const { return Platform::cuda; }

      static Cuda get_default()
      {
        static Cuda c([] {
          cudaStream_t s;
#if CAMP_USE_PLATFORM_DEFAULT_STREAM
          s = 0;
#else
          CAMP_CUDA_API_INVOKE_AND_CHECK(cudaStreamCreate, &s);
#endif
          return s;
        }());
        return c;
      }

      CudaEvent get_event()
      {
        auto d{device_guard(get_device())};
        return CudaEvent(get_stream());
      }

      Event get_event_erased() { return Event{get_event()}; }

      void wait()
      {
        auto d{device_guard(device)};
        CAMP_CUDA_API_INVOKE_AND_CHECK(cudaStreamSynchronize, stream);
      }

      void wait_for(CudaEvent const& e)
      {
        auto d{device_guard(device)};
        CAMP_CUDA_API_INVOKE_AND_CHECK(cudaStreamWaitEvent,
                                       get_stream(),
                                       e.getCudaEvent_t(),
                                       0);
      }

      void wait_for(Event const& e)
      {
        if (auto cuda_event = e.try_get<CudaEvent>()) {
          wait_for(*cuda_event);
        } else {
          e.wait();
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
              CAMP_CUDA_API_INVOKE_AND_CHECK(cudaMalloc,
                                             (void **)&ret,
                                             sizeof(T) * size);
              break;
            case MemoryAccess::Pinned:
              // TODO: do a test here for whether managed is *actually* shared
              // so we can use the better performing memory
              CAMP_CUDA_API_INVOKE_AND_CHECK(cudaMallocHost,
                                             (void **)&ret,
                                             sizeof(T) * size);
              break;
            case MemoryAccess::Managed:
              CAMP_CUDA_API_INVOKE_AND_CHECK(cudaMallocManaged,
                                             (void **)&ret,
                                             sizeof(T) * size);
              break;
          }
        }
        return ret;
      }

      void *calloc(size_t size, MemoryAccess ma = MemoryAccess::Device)
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
            CAMP_CUDA_API_INVOKE_AND_CHECK(cudaFree, p);
            break;
          case MemoryAccess::Pinned:
            // TODO: do a test here for whether managed is *actually* shared
            // so we can use the better performing memory
            CAMP_CUDA_API_INVOKE_AND_CHECK(cudaFreeHost, p);
            break;
          case MemoryAccess::Managed:
            CAMP_CUDA_API_INVOKE_AND_CHECK(cudaFree, p);
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
          CAMP_CUDA_API_INVOKE_AND_CHECK(
              cudaMemcpyAsync, dst, src, size, cudaMemcpyDefault, stream);
        }
      }

      void memset(void *p, int val, size_t size)
      {
        if (size > 0) {
          auto d{device_guard(device)};
          CAMP_CUDA_API_INVOKE_AND_CHECK(cudaMemsetAsync, p, val, size, stream);
        }
      }

      cudaStream_t get_stream() const { return stream; }

      int get_device() const { return device; }

      /*
       * \brief Compares two (Cuda) resources to see if they are equal
       *
       * \return True or false depending on if it is the same stream
       */
      friend inline bool operator==(Cuda const& lhs, Cuda const& rhs) = default;

      size_t get_hash() const
      {
        const size_t cuda_type = size_t(get_platform()) << 32;
        size_t stream_hash = std::hash<void *>{}(static_cast<void *>(stream));
        return cuda_type | (stream_hash & 0xFFFFFFFF);
      }

    private:
      cudaStream_t stream;
      int device;
    };

  }  // namespace v1

}  // namespace resources
}  // namespace camp

namespace std
{

/*
 * \brief Specialization of std::hash for camp::resources::CudaEvent
 *
 * Provides a hash function for cuda typed event objects, enabling their use
 * as keys in unordered associative containers (std::unordered_map,
 * std::unordered_set, etc.)
 *
 * \return A size_t hash value
 */
template <>
struct hash<camp::resources::CudaEvent> {
  std::size_t operator()(const camp::resources::CudaEvent &e) const
  {
    return e.get_hash();
  }
};

/*
 * \brief Specialization of std::hash for camp::resources::Cuda
 *
 * Provides a hash function for cuda typed resource objects, enabling their use
 * as keys in unordered associative containers (std::unordered_map,
 * std::unordered_set, etc.)
 *
 * \return A size_t hash value
 */
template <>
struct hash<camp::resources::Cuda> {
  std::size_t operator()(const camp::resources::Cuda &c) const
  {
    return c.get_hash();
  }
};

}  // namespace std

#endif  // #ifdef CAMP_ENABLE_CUDA

#endif /* __CAMP_CUDA_HPP */
