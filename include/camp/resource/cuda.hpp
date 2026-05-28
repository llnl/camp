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
    class Cuda;

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
      CudaEvent(cudaStream_t stream) { init(stream); }

      CudaEvent(Cuda &res);

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
       * \brief Compares two events to see if they are equal
       *
       * \return True or false depending on if this is the same event
       */
      friend inline bool operator==(CudaEvent const& lhs, CudaEvent const& rhs) = default;

      size_t get_hash() const
      {
        const size_t platform_type = size_t(get_platform()) << 32;
        size_t hash = std::hash<cudaEvent_t>{}(m_event);
        return platform_type | (hash & 0xFFFFFFFF);
      }

    private:
      cudaEvent_t m_event;

      void init(cudaStream_t stream)
      {
        CAMP_CUDA_API_INVOKE_AND_CHECK(cudaEventCreateWithFlags,
                                       &m_event,
                                       cudaEventDisableTiming);
        CAMP_CUDA_API_INVOKE_AND_CHECK(cudaEventRecord, m_event, stream);
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

      CudaEvent get_event() { return CudaEvent(*this); }

      Event get_event_erased() { return Event{CudaEvent(*this)}; }

      void wait()
      {
        auto d{device_guard(device)};
        CAMP_CUDA_API_INVOKE_AND_CHECK(cudaStreamSynchronize, stream);
      }

      void wait_for(Event *e)
      {
        auto *cuda_event = e->try_get<CudaEvent>();
        if (cuda_event) {
          auto d{device_guard(device)};
          CAMP_CUDA_API_INVOKE_AND_CHECK(cudaStreamWaitEvent,
                                         get_stream(),
                                         cuda_event->getCudaEvent_t(),
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
              CAMP_CUDA_API_INVOKE_AND_CHECK(cudaMalloc,
                                             &ret,
                                             sizeof(T) * size);
              break;
            case MemoryAccess::Pinned:
              // TODO: do a test here for whether managed is *actually* shared
              // so we can use the better performing memory
              CAMP_CUDA_API_INVOKE_AND_CHECK(cudaMallocHost,
                                             &ret,
                                             sizeof(T) * size);
              break;
            case MemoryAccess::Managed:
              CAMP_CUDA_API_INVOKE_AND_CHECK(cudaMallocManaged,
                                             &ret,
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

    inline CudaEvent::CudaEvent(Cuda &res)
    {
      auto d{device_guard(res.get_device())};
      init(res.get_stream());
    }

  }  // namespace v1

  template <>
  struct is_concrete_resource_impl<Cuda> : std::true_type {
  };

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
