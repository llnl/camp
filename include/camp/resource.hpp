//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef __CAMP_RESOURCE_HPP
#define __CAMP_RESOURCE_HPP

#include <cstring>
#include <memory>
#include <mutex>
#include <type_traits>

#include "camp/concepts.hpp"
#include "camp/config.hpp"
#include "camp/defines.hpp"
#include "camp/helpers.hpp"

#include "camp/resource/event.hpp"

// last to ensure we don't hide breakage in the others
#include "camp/resource/platform.hpp"

namespace camp
{
namespace resources
{
  inline namespace v1
  {
    class Resource
    {
    public:
      using event_type = Event;

      Resource() = default;
      Resource(Resource &&) = default;
      Resource(Resource const &) = default;
      Resource &operator=(Resource &&) = default;
      Resource &operator=(Resource const &) = default;

      template <camp::concepts::ConcreteResource T>
      Resource(T &&value)
      {
        m_value.reset(new ContextModel<type::ref::rem<T>>(forward<T>(value)));
      }

      template <typename T>
      T* try_get()
      {
        if (!m_value) {
          return nullptr;
        }
        auto result = dynamic_cast<ContextModel<T> *>(m_value.get());
        if (!result) {
          return nullptr;
        }
        return result->get();
      }

      template <typename T>
      T const* try_get() const
      {
        if (!m_value) {
          return nullptr;
        }
        auto result = dynamic_cast<ContextModel<T> *>(m_value.get());
        if (!result) {
          return nullptr;
        }
        return result->get();
      }

      template <typename T>
      T& get() &
      {
        T* result = try_get<T>();
        if (result == nullptr) {
          ::camp::throw_re("Incompatible Resource type get cast.");
        }
        return *result;
      }

      template <typename T>
      T const& get() const&
      {
        T const* result = try_get<T>();
        if (result == nullptr) {
          ::camp::throw_re("Incompatible Resource type get cast.");
        }
        return *result;
      }

      template <typename T>
      T get() &&
      {
        T* result = try_get<T>();
        if (result == nullptr) {
          ::camp::throw_re("Incompatible Resource type get cast.");
        }
        return std::move(*result);
      }

      Platform get_platform() const
      {
        return m_value ? m_value->get_platform() : Platform::undefined;
      }

      template <typename T>
      T *allocate(size_t size, MemoryAccess ma = MemoryAccess::Device)
      {
        if (size == 0) {
          return nullptr;
        }
        if (!m_value) {
          ::camp::throw_re("Empty Resource type allocate call.");
        }
        return (T *)m_value->allocate(size * sizeof(T), ma);
      }

      void *calloc(size_t size, MemoryAccess ma = MemoryAccess::Device)
      {
        if (size == 0) {
          return nullptr;
        }
        if (!m_value) {
          ::camp::throw_re("Empty Resource type calloc call.");
        }
        return m_value->calloc(size, ma);
      }

      void deallocate(void *p, MemoryAccess ma = MemoryAccess::Device)
      {
        if (p == nullptr) {
          return;
        }
        if (!m_value) {
          ::camp::throw_re("Empty Resource type deallocate call.");
        }
        m_value->deallocate(p, ma);
      }

      void memcpy(void *dst, const void *src, size_t size)
      {
        if (size == 0) {
          return;
        }
        if (!m_value) {
          ::camp::throw_re("Empty Resource type memcpy call.");
        }
        m_value->memcpy(dst, src, size);
      }

      void memset(void *p, int val, size_t size)
      {
        if (size == 0) {
          return;
        }
        if (!m_value) {
          ::camp::throw_re("Empty Resource type memset call.");
        }
        m_value->memset(p, val, size);
      }

      Event get_event()
      {
        if (!m_value) {
          return Event{};
        }
        return m_value->get_event();
      }

      Event get_event_erased()
      {
        if (!m_value) {
          return Event{};
        }
        return m_value->get_event_erased();
      }

      void wait_for(Event const& e)
      {
        if (!m_value) {
            e.wait();
          return;
        }
        m_value->wait_for(e);
      }

      void wait()
      {
        if (!m_value) {
          return;
        }
        m_value->wait();
      }

      explicit operator bool() const { return static_cast<bool>(m_value); }

      /*
       * \brief Compares two Resources to see if they are equal. Two Resources
       * are equal if they have the platform and same stream/queue
       *
       * \return True if they have the same platform and stream/queue, false
       * otherwise.
       */
      friend inline bool operator==(Resource const &lhs, Resource const &rhs)
      {
        if (!lhs.m_value && !rhs.m_value) {
          return true;
        }
        if ((!lhs.m_value && rhs.m_value) ||
            (lhs.m_value && !rhs.m_value)) {
          return false;
        }
        if (lhs.get_platform() == rhs.get_platform()) {
          return lhs.m_value->compare(rhs);
        }
        return false;
      }

      template < camp::concepts::ConcreteResource Res >
      friend inline bool operator==(Resource const &lhs, Res const &rhs)
      {
        if (!lhs.m_value) {
          return false;
        }
        if (lhs.get_platform() == rhs.get_platform()) {
          if (auto lhs_res = lhs.try_get<Res>()) {
            return *lhs_res == rhs;
          }
        }
        return false;
      }

    private:
      friend struct std::hash<camp::resources::Resource>;

      /*
       * \brief Retrieves the a hash for this Resource.
       * The hash allows Resources to be used as keys in data structures
       * like unordered maps.
       *
       * \return A size_t hash value for this Resource's
       * platform and stream/queue combination.
       *
       */
      size_t get_hash() const { return m_value ? m_value->get_hash() : 0u; }

      class ContextInterface
      {
      public:
        virtual ~ContextInterface() {}

        virtual Platform get_platform() const = 0;

        virtual bool compare(Resource const &r) const = 0;
        virtual size_t get_hash() const = 0;

        virtual void *allocate(size_t size,
                               MemoryAccess ma = MemoryAccess::Device) = 0;
        virtual void *calloc(size_t size,
                             MemoryAccess ma = MemoryAccess::Device) = 0;
        virtual void deallocate(void *p,
                                MemoryAccess ma = MemoryAccess::Device) = 0;
        virtual void memcpy(void *dst, const void *src, size_t size) = 0;
        virtual void memset(void *p, int val, size_t size) = 0;

        virtual Event get_event() = 0;
        virtual Event get_event_erased() = 0;
        virtual void wait_for(Event const& e) = 0;
        virtual void wait() = 0;
      };

      template <typename T>
      class ContextModel final : public ContextInterface
      {
      public:
        explicit ContextModel(T const &modelVal) : m_modelVal(modelVal) {}

        Platform get_platform() const override
        {
          return m_modelVal.get_platform();
        }

        bool compare(Resource const &r) const override
        {
          return m_modelVal == r.get<T>();
        }

        size_t get_hash() const override { return m_modelVal.get_hash(); }

        void *allocate(size_t size,
                       MemoryAccess ma = MemoryAccess::Device) override
        {
          return m_modelVal.template allocate<char>(size, ma);
        }

        void *calloc(size_t size,
                     MemoryAccess ma = MemoryAccess::Device) override
        {
          return m_modelVal.calloc(size, ma);
        }

        void deallocate(void *p,
                        MemoryAccess ma = MemoryAccess::Device) override
        {
          m_modelVal.deallocate(p, ma);
        }

        void memcpy(void *dst, const void *src, size_t size) override
        {
          m_modelVal.memcpy(dst, src, size);
        }

        void memset(void *p, int val, size_t size) override
        {
          m_modelVal.memset(p, val, size);
        }

        Event get_event() override { return m_modelVal.get_event_erased(); }

        Event get_event_erased() override
        {
          return m_modelVal.get_event_erased();
        }

        void wait_for(Event const& e) override { m_modelVal.wait_for(e); }

        void wait() override { m_modelVal.wait(); }

        const T* get() const { return &m_modelVal; }

        T* get() { return &m_modelVal; }

      private:
        T m_modelVal;
      };

      std::shared_ptr<ContextInterface> m_value;
    };

    template <typename Res>
    struct EventProxy : ::camp::resources::detail::EventProxyBase
    {
      using native_event = typename Res::event_type;

      EventProxy(EventProxy &&) = default;
      EventProxy(EventProxy const &) = delete;
      EventProxy &operator=(EventProxy &&) = default;
      EventProxy &operator=(EventProxy const &) = delete;

      EventProxy(Res r) : resource_{move(r)} {}

      native_event get()
      requires (!std::same_as<native_event, Event>)
      {
        return resource_.get_event();
      }

      Event get()
      requires (std::same_as<native_event, Event>)
      {
        return resource_.get_event_erased();
      }

      operator native_event()
      requires (!std::same_as<native_event, Event>)
      {
        return resource_.get_event();
      }

      operator Event()
      {
        return resource_.get_event_erased();
      }

      Res resource_;
    };

  }  // namespace v1
}  // namespace resources
}  // namespace camp

namespace std
{

/*
 * \brief Specialization of std::hash for camp::resources::Resource
 *
 * Provides a hash function for Resource objects, enabling their use as keys
 * in unordered associative containers (std::unordered_map, std::unordered_set,
 * etc.)
 *
 * \return A size_t hash value
 */
template <>
struct hash<camp::resources::Resource> {
  std::size_t operator()(const camp::resources::Resource &r) const
  {
    return r.get_hash();
  }
};

}  // namespace std

#include "camp/resource/host.hpp"

#if defined(CAMP_HAVE_CUDA)
#include "camp/resource/cuda.hpp"
#endif
#if defined(CAMP_HAVE_HIP)
#include "camp/resource/hip.hpp"
#endif
#if defined(CAMP_HAVE_SYCL)
#include "camp/resource/sycl.hpp"
#endif

#if defined(CAMP_HAVE_OMP_OFFLOAD)
#include "camp/resource/omp_target.hpp"
#endif

#endif /* __CAMP_RESOURCE_HPP */
