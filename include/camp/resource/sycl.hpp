//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef __CAMP_SYCL_HPP
#define __CAMP_SYCL_HPP

#include "camp/config.hpp"

#ifdef CAMP_ENABLE_SYCL

#include <array>
#include <map>
#include <mutex>
#include <sycl/sycl.hpp>

#include "camp/defines.hpp"
#include "camp/resource/event.hpp"
#include "camp/resource/platform.hpp"

namespace camp
{
namespace resources
{
  inline namespace v1
  {
    class Sycl;

    class SyclEvent
    {
    public:
      explicit SyclEvent(sycl::event e)
        : m_event(std::move(e))
      {}

      // TODO: see what overhead an empty submit has
      SyclEvent(sycl::queue& qu)
      {
        if (!qu.is_in_order()) {
          ::camp::throw_re("Queue is not in_order.");
        }
        m_event = qu.submit([&](::sycl::handler& CAMP_UNUSED_ARG(h)) {});
      }

      SyclEvent(Sycl& res);

      Platform get_platform() const { return Platform::sycl; }

      bool check() const
      {
        return m_event.get_info<sycl::info::event::command_execution_status>()
            == sycl::info::event_command_status::complete;
      }

      void wait() const { m_event.wait(); } // sycl::event::wait is non-const

      sycl::event& getSyclEvent_t() { return m_event; }

      sycl::event const& getSyclEvent_t() const { return m_event; }

      /*
       * \brief Compares two events to see if they are equal
       *
       * \return True or false depending on if this is the same event
       */
      friend inline bool operator==(SyclEvent const& lhs, SyclEvent const& rhs) = default;

      size_t get_hash() const
      {
        const size_t sycl_type = size_t(get_platform()) << 32;
        size_t stream_hash = std::hash<sycl::event>{}(m_event);
        return sycl_type | (stream_hash & 0xFFFFFFFF);
      }

    private:
      mutable sycl::event m_event; // mutable as use non-const member function
    };

    class Sycl
    {
      /*
       * \brief Get the camp managed sycl context.
       *
       * Note that the first call sets up the context with the given argument.
       *
       * \return Reference to the camp managed sycl context.
       */
      static sycl::context& get_private_context(
          const sycl::context* syclContext)
      {
        static sycl::context s_context(syclContext ? *syclContext
                                                   : sycl::context());
        return s_context;
      }

      /*
       * \brief Get the per thread camp managed sycl context.
       *
       * Note that the first call sets up the context with the given argument.
       *
       * \return Reference to the per thread camp managed sycl context.
       */
      static sycl::context& get_thread_private_context(
          sycl::context const& syclContext)
      {
        thread_local sycl::context t_context(syclContext);
        return t_context;
      }

      /*
       * \brief Get the per thread camp managed sycl context.
       *
       * Note that the first call sets up the context with the given argument.
       *
       * \return Reference to the per thread camp managed sycl context.
       */
      static sycl::context const& get_thread_default_context(
          sycl::context const& syclContext)
      {
        get_private_context(&syclContext);
        return get_thread_private_context(syclContext);
      }

    public:
      /*
       * \brief Get the camp managed sycl context.
       *
       * \return Const reference to the camp managed sycl context.
       */
      static sycl::context const& get_default_context()
      {
        return get_private_context(nullptr);
      }

      /*
       * \brief Get the per thread camp managed sycl context.
       *
       * \return Const reference to the per thread camp managed sycl context.
       */
      static sycl::context const& get_thread_default_context()
      {
        return get_thread_private_context(get_private_context(nullptr));
      }

      /*
       * \brief Set the camp managed sycl context.
       */
      static void set_default_context(sycl::context const& syclContext)
      {
        get_private_context(&syclContext) = syclContext;
      }

      /*
       * \brief Set the per thread camp managed sycl context.
       */
      static void set_thread_default_context(sycl::context const& syclContext)
      {
        get_private_context(&syclContext);
        get_thread_private_context(syclContext) = syclContext;
      }

    private:
      static sycl::queue& get_a_queue(const sycl::context* syclContext, int num)
      {
        static constexpr int num_queues = 16;

        static std::mutex s_mtx;

        // note that this type must not invalidate iterators when modified
        using value_second_type =
            std::pair<int, std::array<sycl::queue, num_queues>>;
        using queueMap_type = std::map<const sycl::context*, value_second_type>;
        static queueMap_type queueMap;
        static const typename queueMap_type::iterator queueMap_end =
            queueMap.end();
        thread_local typename queueMap_type::iterator cachedContextIter =
            queueMap_end;

        if (syclContext) {
          // implement sticky contexts
          set_thread_default_context(*syclContext);
        }
        syclContext = &get_thread_default_context();

        if (syclContext != cachedContextIter->first) {
          cachedContextIter = queueMap_end;
        }

        if (cachedContextIter == queueMap_end || num < 0) {
          std::lock_guard<std::mutex> lock(s_mtx);

          if (cachedContextIter == queueMap_end) {
            cachedContextIter = queueMap.find(syclContext);
            if (cachedContextIter == queueMap_end) {
              static constexpr auto gpuSelector = sycl::gpu_selector_v;
              static const sycl::property_list propertyList =
                  sycl::property_list(sycl::property::queue::in_order());

              cachedContextIter =
                  queueMap
                      .emplace(syclContext,
                               value_second_type(num_queues - 1,
                                                 {sycl::queue(*syclContext,
                                                              gpuSelector,
                                                              propertyList),
                                                  sycl::queue(*syclContext,
                                                              gpuSelector,
                                                              propertyList),
                                                  sycl::queue(*syclContext,
                                                              gpuSelector,
                                                              propertyList),
                                                  sycl::queue(*syclContext,
                                                              gpuSelector,
                                                              propertyList),
                                                  sycl::queue(*syclContext,
                                                              gpuSelector,
                                                              propertyList),
                                                  sycl::queue(*syclContext,
                                                              gpuSelector,
                                                              propertyList),
                                                  sycl::queue(*syclContext,
                                                              gpuSelector,
                                                              propertyList),
                                                  sycl::queue(*syclContext,
                                                              gpuSelector,
                                                              propertyList),
                                                  sycl::queue(*syclContext,
                                                              gpuSelector,
                                                              propertyList),
                                                  sycl::queue(*syclContext,
                                                              gpuSelector,
                                                              propertyList),
                                                  sycl::queue(*syclContext,
                                                              gpuSelector,
                                                              propertyList),
                                                  sycl::queue(*syclContext,
                                                              gpuSelector,
                                                              propertyList),
                                                  sycl::queue(*syclContext,
                                                              gpuSelector,
                                                              propertyList),
                                                  sycl::queue(*syclContext,
                                                              gpuSelector,
                                                              propertyList),
                                                  sycl::queue(*syclContext,
                                                              gpuSelector,
                                                              propertyList),
                                                  sycl::queue(*syclContext,
                                                              gpuSelector,
                                                              propertyList)}))
                      .first;
            }
          }

          if (num < 0) {
            int& previous = cachedContextIter->second.first;
            previous = (previous + 1) % num_queues;
            return cachedContextIter->second.second[previous];
          }
        }

        return cachedContextIter->second.second[num % num_queues];
      }

      // Private from-queue constructor
      Sycl(sycl::queue& q) : qu(q) {}

    public:
      Sycl(int group = -1,
           sycl::context const& syclContext = get_thread_default_context())
          : qu(get_a_queue(&syclContext, group))
      {
      }

      /// Create a resource from a custom queue
      static Sycl SyclFromQueue(sycl::queue& q) { return Sycl(q); }

      // get default resource
      static Sycl get_default() { return Sycl(0, get_default_context()); }

      // Methods
      Platform get_platform() const { return Platform::sycl; }

      // Event
      SyclEvent get_event() { return SyclEvent(*this); }

      Event get_event_erased() { return Event{SyclEvent(*this)}; }

      void wait() { qu.wait(); }

      void wait_for(Event* e)
      {
        auto* sycl_event = e->try_get<SyclEvent>();
        if (sycl_event) {
          qu.submit([&](::sycl::handler& h) {
            h.depends_on(sycl_event->getSyclEvent_t());
          });
        } else {
          e->wait();
        }
      }

      // Memory
      template <typename T>
      T* allocate(size_t size, MemoryAccess ma = MemoryAccess::Device)
      {
        T* ret = nullptr;
        if (size > 0) {
          ret = sycl::malloc_shared<T>(size, qu);
          switch (ma) {
            case MemoryAccess::Unknown:
            case MemoryAccess::Device:
              ret = sycl::malloc_device<T>(size, qu);
              break;
            case MemoryAccess::Pinned:
              ret = sycl::malloc_host<T>(size, qu);
              break;
            case MemoryAccess::Managed:
              ret = sycl::malloc_shared<T>(size, qu);
              break;
          }
        }
        return ret;
      }

      void* calloc(size_t size, MemoryAccess ma = MemoryAccess::Device)
      {
        void* p = allocate<char>(size, ma);
        this->memset(p, 0, size);
        return p;
      }

      void deallocate(void* p, MemoryAccess ma = MemoryAccess::Device)
      {
        CAMP_ALLOW_UNUSED_LOCAL(ma);
        sycl::free(p, qu);
      }

      void memcpy(void* dst, const void* src, size_t size)
      {
        if (size > 0) {
          qu.memcpy(dst, src, size).wait();
        }
      }

      void memset(void* p, int val, size_t size)
      {
        if (size > 0) {
          qu.memset(p, val, size).wait();
        }
      }

      // implementation specific
      sycl::queue& get_queue() { return qu; }

      sycl::queue const& get_queue() const { return qu; }

      /*
       * \brief Compares two (Sycl) resources to see if they are equal
       *
       * \return True or false depending on if this is the same queue
       */
      friend inline bool operator==(Sycl const& lhs, Sycl const& rhs) = default;

      size_t get_hash() const
      {
        const size_t sycl_type = size_t(get_platform()) << 32;
        size_t stream_hash = std::hash<sycl::queue>{}(qu);
        return sycl_type | (stream_hash & 0xFFFFFFFF);
      }

    private:
      sycl::queue qu;
    };

    inline SyclEvent::SyclEvent(Sycl &res)
      : SyclEvent(res.get_queue())
    {}

  }  // namespace v1

  template <>
  struct is_concrete_resource_impl<Sycl> : std::true_type {
  };
}  // namespace resources
}  // namespace camp

namespace std
{

/*
 * \brief Specialization of std::hash for camp::resources::SyclEvent
 *
 * Provides a hash function for sycl typed event objects, enabling their use
 * as keys in unordered associative containers (std::unordered_map,
 * std::unordered_set, etc.)
 *
 * \return A size_t hash value
 */
template <>
struct hash<camp::resources::SyclEvent> {
  std::size_t operator()(const camp::resources::SyclEvent& e) const
  {
    return e.get_hash();
  }
};

/*
 * \brief Specialization of std::hash for camp::resources::Sycl
 *
 * Provides a hash function for Sycl typed resource objects, enabling their use
 * as keys in unordered associative containers (std::unordered_map,
 * std::unordered_set, etc.)
 *
 * \return A size_t hash value
 */
template <>
struct hash<camp::resources::Sycl> {
  std::size_t operator()(const camp::resources::Sycl& s) const
  {
    return s.get_hash();
  }
};

}  // namespace std
#endif  // #ifdef CAMP_ENABLE_SYCL

#endif /* __CAMP_SYCL_HPP */
