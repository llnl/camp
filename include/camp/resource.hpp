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

#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>

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
    class Resource;

    namespace detail
    {
      struct ResourceVTable {
        std::type_info const* type;
        Platform (*platform)(void const*) noexcept;
        bool (*equal)(void const*, void const*) noexcept;
        std::size_t (*hash)(void const*) noexcept;
        std::shared_ptr<void> (*copy)(void const*);
        void* (*allocate)(void*, std::size_t, MemoryAccess);
        void* (*calloc)(void*, std::size_t, MemoryAccess);
        void (*deallocate)(void*, void*, MemoryAccess);
        void (*memcpy)(void*, void*, void const*, std::size_t);
        void (*memset)(void*, void*, int, std::size_t);
        Event (*get_event)(void*);
        void (*wait_for)(void*, EventConstRef);
        void (*wait)(void*);
      };

      template <typename T>
      inline constexpr ResourceVTable resource_vtable_for{
          &typeid(T),
          [](void const* p) noexcept {
            static_assert(noexcept(std::declval<T const&>().get_platform()),
                          "Concrete resource get_platform() must be "
                          "noexcept");
            return static_cast<T const*>(p)->get_platform();
          },
          [](void const* lhs, void const* rhs) noexcept {
            static_assert(noexcept(std::declval<T const&>()
                                   == std::declval<T const&>()),
                          "Concrete resource equality must be noexcept");
            return *static_cast<T const*>(lhs) == *static_cast<T const*>(rhs);
          },
          [](void const* p) noexcept {
            static_assert(noexcept(std::declval<T const&>().get_hash()),
                          "Concrete resource get_hash() must be noexcept");
            return static_cast<T const*>(p)->get_hash();
          },
          [](void const* p) -> std::shared_ptr<void> {
            static_assert(std::is_copy_constructible_v<T>,
                          "Concrete resources must be copy constructible");
            return std::make_shared<T>(*static_cast<T const*>(p));
          },
          [](void* p, std::size_t size, MemoryAccess access) -> void* {
            return static_cast<T*>(p)->template allocate<char>(size, access);
          },
          [](void* p, std::size_t size, MemoryAccess access) {
            return static_cast<T*>(p)->calloc(size, access);
          },
          [](void* p, void* memory, MemoryAccess access) {
            static_cast<T*>(p)->deallocate(memory, access);
          },
          [](void* p, void* dst, void const* src, std::size_t size) {
            static_cast<T*>(p)->memcpy(dst, src, size);
          },
          [](void* p, void* memory, int value, std::size_t size) {
            static_cast<T*>(p)->memset(memory, value, size);
          },
          [](void* p) { return static_cast<T*>(p)->get_event_erased(); },
          [](void* p, EventConstRef event) {
            using event_type = typename T::event_type;
            if (event_type const* typed_event =
                    event.template try_get<event_type>()) {
              static_cast<T*>(p)->wait_for(*typed_event);
            } else {
              event.wait();
            }
          },
          [](void* p) { static_cast<T*>(p)->wait(); }};

      /** \brief Implementation of mutable and const non-owning resource views.
       */
      template <bool IsConst>
      struct BasicResourceRef {
        using event_type = Event;
        using pointer = std::conditional_t<IsConst, void const*, void*>;

        BasicResourceRef() = default;

        template <camp::concepts::ConcreteResource T>
        BasicResourceRef(T& value) noexcept
            : m_resource(std::addressof(value)),
              m_vtable(&resource_vtable_for<T>)
        {
        }

        template <camp::concepts::ConcreteResource T>
        BasicResourceRef(T const& value) noexcept
          requires(IsConst)
            : m_resource(std::addressof(value)),
              m_vtable(&resource_vtable_for<T>)
        {
        }

        template <camp::concepts::ConcreteResource T>
        BasicResourceRef(T const&)
          requires(!IsConst)
        = delete;

        template <camp::concepts::ConcreteResource T>
        BasicResourceRef(T&&)
          requires std::is_rvalue_reference_v<T&&>
        = delete;

        BasicResourceRef(Resource& resource) noexcept;
        BasicResourceRef(Resource const& resource) noexcept
          requires(IsConst);
        BasicResourceRef(Resource&&) = delete;
        BasicResourceRef(Resource const&&) = delete;

        template <bool OtherConst>
        BasicResourceRef(BasicResourceRef<OtherConst> other) noexcept
          requires(IsConst && !OtherConst)
            : m_resource(other.m_resource), m_vtable(other.m_vtable)
        {
        }

        template <typename T>
        using access_type = std::conditional_t<IsConst, T const, T>;

        template <typename T>
        access_type<T>* try_get() const noexcept
        {
          if (!m_resource || *m_vtable->type != typeid(T)) {
            return nullptr;
          }
          return static_cast<access_type<T>*>(m_resource);
        }

        template <typename T>
        access_type<T>& get() const
        {
          auto* result = try_get<T>();
          if (result == nullptr) {
            ::camp::throw_re("Incompatible Resource type get cast.");
          }
          return *result;
        }

        Platform get_platform() const noexcept
        {
          return m_resource ? m_vtable->platform(m_resource)
                            : Platform::undefined;
        }

        template <typename T>
        T* allocate(std::size_t count,
                    MemoryAccess access = MemoryAccess::Device) const
          requires(!IsConst)
        {
          if (count == 0) {
            return nullptr;
          }
          if (!m_resource) {
            ::camp::throw_re("Empty Resource type allocate call.");
          }
          return static_cast<T*>(
              m_vtable->allocate(m_resource, count * sizeof(T), access));
        }

        void* calloc(std::size_t size,
                     MemoryAccess access = MemoryAccess::Device) const
          requires(!IsConst)
        {
          if (size == 0) {
            return nullptr;
          }
          if (!m_resource) {
            ::camp::throw_re("Empty Resource type calloc call.");
          }
          return m_vtable->calloc(m_resource, size, access);
        }

        void deallocate(void* memory,
                        MemoryAccess access = MemoryAccess::Device) const
          requires(!IsConst)
        {
          if (!memory) {
            return;
          }
          if (!m_resource) {
            ::camp::throw_re("Empty Resource type deallocate call.");
          }
          m_vtable->deallocate(m_resource, memory, access);
        }

        void memcpy(void* dst, void const* src, std::size_t size) const
          requires(!IsConst)
        {
          if (size == 0) {
            return;
          }
          if (!m_resource) {
            ::camp::throw_re("Empty Resource type memcpy call.");
          }
          m_vtable->memcpy(m_resource, dst, src, size);
        }

        void memset(void* memory, int value, std::size_t size) const
          requires(!IsConst)
        {
          if (size == 0) {
            return;
          }
          if (!m_resource) {
            ::camp::throw_re("Empty Resource type memset call.");
          }
          m_vtable->memset(m_resource, memory, value, size);
        }

        Event get_event() const
          requires(!IsConst)
        {
          return m_resource ? m_vtable->get_event(m_resource) : Event{};
        }

        Event get_event_erased() const
          requires(!IsConst)
        {
          return get_event();
        }

        void wait_for(EventConstRef event) const
          requires(!IsConst)
        {
          if (m_resource) {
            m_vtable->wait_for(m_resource, event);
          } else {
            event.wait();
          }
        }

        template <camp::concepts::ConcreteEvent T>
        void wait_for(T const& event) const
          requires(!IsConst)
        {
          wait_for(EventConstRef{event});
        }

        void wait_for(Event const& event) const
          requires(!IsConst)
        {
          wait_for(EventConstRef{event});
        }

        void wait() const
          requires(!IsConst)
        {
          if (m_resource) {
            m_vtable->wait(m_resource);
          }
        }

        explicit operator bool() const noexcept
        {
          return m_resource != nullptr;
        }

        template <bool OtherConst>
        bool operator==(BasicResourceRef<OtherConst> rhs) const noexcept
        {
          if (!m_resource || !rhs.m_resource) {
            return m_resource == rhs.m_resource;
          }
          return *m_vtable->type == *rhs.m_vtable->type
                 && m_vtable->equal(m_resource, rhs.m_resource);
        }

        template <camp::concepts::ConcreteResource T>
        bool operator==(T const& rhs) const
            noexcept(noexcept(*this->template try_get<T>() == rhs))
        {
          if (auto* value = this->template try_get<T>()) {
            return *value == rhs;
          }
          return false;
        }

      private:
        template <bool>
        friend struct BasicResourceRef;
        friend class ::camp::resources::Resource;
        friend struct std::hash<BasicResourceRef<IsConst>>;

        std::size_t get_hash() const noexcept
        {
          return m_resource ? m_vtable->hash(m_resource) : 0u;
        }

        pointer m_resource = nullptr;
        ResourceVTable const* m_vtable = nullptr;
      };
    }  // namespace detail

    /**
     * \brief A mutable, non-owning, type-erased resource view.
     *
     * ResourceRef borrows mutable concrete resources or mutable owning
     * Resource objects. It provides mutable typed access and resource
     * operations, and converts implicitly to ResourceConstRef. It cannot
     * borrow const objects or temporaries and does not extend the lifetime of
     * its target.
     */
    using ResourceRef = detail::BasicResourceRef<false>;

    /**
     * \brief A const, non-owning, type-erased resource view.
     *
     * ResourceConstRef borrows mutable or const concrete resources and owning
     * Resource objects. It provides only observation and const typed access.
     * It cannot borrow temporaries and does not extend the lifetime of its
     * target.
     */
    using ResourceConstRef = detail::BasicResourceRef<true>;

    /**
     * \brief An owning, type-erased resource.
     *
     * Copies share ownership of the stored concrete resource.
     */
    class Resource : private ResourceRef
    {
      template <typename T>
      Resource(std::shared_ptr<T> owner)
          : ResourceRef{*owner}, m_owner{std::move(owner)}
      {
      }

    public:
      using event_type = Event;

      Resource() = default;
      Resource(Resource const&) = default;

      Resource(Resource&& other) noexcept : m_owner(std::move(other.m_owner))
      {
        auto& this_ref = static_cast<ResourceRef&>(*this);
        auto& other_ref = static_cast<ResourceRef&>(other);
        this_ref = std::exchange(other_ref, ResourceRef{});
      }

      Resource& operator=(Resource const&) = default;

      Resource& operator=(Resource&& other) noexcept
      {
        if (this != &other) {
          auto& this_ref = static_cast<ResourceRef&>(*this);
          auto& other_ref = static_cast<ResourceRef&>(other);
          this_ref = std::exchange(other_ref, ResourceRef{});
          m_owner = std::move(other.m_owner);
        }
        return *this;
      }

      template <camp::concepts::ConcreteResource T>
      Resource(T&& value)
          : Resource(std::make_shared<camp::decay<T>>(std::forward<T>(value)))
      {
      }

      /**
       * \brief Create an owning type-erased resource containing a copy of the
       * referenced concrete resource.
       *
       * An empty resource reference produces an empty Resource.
       */
      static Resource from_ref(ResourceConstRef resource)
      {
        Resource result;
        if (!resource.m_resource) {
          return result;
        }

        result.m_owner = resource.m_vtable->copy(resource.m_resource);
        result.m_resource = result.m_owner.get();
        result.m_vtable = resource.m_vtable;
        return result;
      }

      using ResourceRef::get_platform;
      using ResourceRef::operator bool;

      template <typename T>
      T* try_get() noexcept
      {
        return ResourceRef::try_get<T>();
      }

      template <typename T>
      T const* try_get() const noexcept
      {
        return ResourceConstRef{*this}.template try_get<T>();
      }

      template <typename T>
      T& get() &
      {
        return ResourceRef::get<T>();
      }

      template <typename T>
      T const& get() const&
      {
        return ResourceConstRef{*this}.template get<T>();
      }

      template <typename T>
      T get() &&
      {
        return std::move(ResourceRef::get<T>());
      }

      template <typename T>
      T* allocate(std::size_t count, MemoryAccess access = MemoryAccess::Device)
      {
        return ResourceRef::allocate<T>(count, access);
      }

      void* calloc(std::size_t size, MemoryAccess access = MemoryAccess::Device)
      {
        return ResourceRef::calloc(size, access);
      }

      void deallocate(void* memory, MemoryAccess access = MemoryAccess::Device)
      {
        ResourceRef::deallocate(memory, access);
      }

      void memcpy(void* dst, void const* src, std::size_t size)
      {
        ResourceRef::memcpy(dst, src, size);
      }

      void memset(void* memory, int value, std::size_t size)
      {
        ResourceRef::memset(memory, value, size);
      }

      Event get_event() { return ResourceRef::get_event(); }

      Event get_event_erased() { return ResourceRef::get_event_erased(); }

      void wait_for(EventConstRef event) { ResourceRef::wait_for(event); }

      template <camp::concepts::ConcreteEvent T>
      void wait_for(T const& event)
      {
        ResourceRef::wait_for(event);
      }

      void wait_for(Event const& event) { ResourceRef::wait_for(event); }

      void wait() { ResourceRef::wait(); }

      friend bool operator==(Resource const& lhs, Resource const& rhs) noexcept
      {
        return ResourceConstRef{lhs} == ResourceConstRef{rhs};
      }

      template <bool IsConst>
      friend bool operator==(Resource const& lhs,
                             detail::BasicResourceRef<IsConst> rhs) noexcept
      {
        return ResourceConstRef{lhs} == rhs;
      }

      template <camp::concepts::ConcreteResource T>
      friend bool operator==(Resource const& lhs, T const& rhs) noexcept(
          noexcept(ResourceConstRef{lhs} == rhs))
      {
        return ResourceConstRef{lhs} == rhs;
      }

    private:
      template <bool>
      friend struct detail::BasicResourceRef;
      friend struct std::hash<camp::resources::Resource>;

      std::size_t get_hash() const noexcept
      {
        return ResourceConstRef{*this}.get_hash();
      }

      std::shared_ptr<void> m_owner;
    };

    template <bool IsConst>
    inline detail::BasicResourceRef<IsConst>::BasicResourceRef(
        Resource& resource) noexcept
        : BasicResourceRef(static_cast<ResourceRef const&>(resource))
    {
    }

    template <bool IsConst>
    inline detail::BasicResourceRef<IsConst>::BasicResourceRef(
        Resource const& resource) noexcept
      requires(IsConst)
        : BasicResourceRef(
              ResourceConstRef{static_cast<ResourceRef const&>(resource)})
    {
    }

    template <typename Res>
    struct EventProxy : ::camp::resources::detail::EventProxyBase {
      using native_event = typename Res::event_type;

      EventProxy(EventProxy&&) = default;
      EventProxy(EventProxy const&) = delete;
      EventProxy& operator=(EventProxy&&) = default;
      EventProxy& operator=(EventProxy const&) = delete;

      EventProxy(Res r) : resource_{move(r)} {}

      native_event get()
        requires(!std::same_as<native_event, Event>)
      {
        return resource_.get_event();
      }

      Event get()
        requires(std::same_as<native_event, Event>)
      {
        return resource_.get_event_erased();
      }

      operator native_event()
        requires(!std::same_as<native_event, Event>)
      {
        return resource_.get_event();
      }

      operator Event() { return resource_.get_event_erased(); }

      Res resource_;
    };

  }  // namespace v1
}  // namespace resources
}  // namespace camp

namespace std
{

/** \brief Hash support for mutable resource views. */
template <>
struct hash<camp::resources::ResourceRef> {
  std::size_t operator()(camp::resources::ResourceRef resource) const noexcept
  {
    return resource.get_hash();
  }
};

/** \brief Hash support for const resource views. */
template <>
struct hash<camp::resources::ResourceConstRef> {
  std::size_t operator()(
      camp::resources::ResourceConstRef resource) const noexcept
  {
    return resource.get_hash();
  }
};

/** \brief Hash support for owning resources. */
template <>
struct hash<camp::resources::Resource> {
  std::size_t operator()(
      camp::resources::Resource const& resource) const noexcept
  {
    return resource.get_hash();
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

namespace camp
{
namespace resources
{
  inline namespace v1
  {
    /**
     * \brief Clean up resources managed by all enabled backends.
     *
     * Existing resources that refer to backend-managed streams are invalid
     * after this call. Caller-owned resources are not affected. The caller
     * must ensure no other thread is using backend resources while cleanup
     * runs.
     */
    inline void cleanup()
    {
      Host::cleanup();
#if defined(CAMP_HAVE_CUDA)
      Cuda::cleanup();
#endif
#if defined(CAMP_HAVE_HIP)
      Hip::cleanup();
#endif
#if defined(CAMP_HAVE_SYCL)
      Sycl::cleanup();
#endif
#if defined(CAMP_HAVE_OMP_OFFLOAD)
      Omp::cleanup();
#endif
    }

  }  // namespace v1
}  // namespace resources
}  // namespace camp

#include "camp/resource/resource_allocator.hpp"

#endif /* __CAMP_RESOURCE_HPP */
