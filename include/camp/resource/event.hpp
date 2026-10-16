//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef __CAMP_EVENT_HPP
#define __CAMP_EVENT_HPP

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

// last to ensure we don't hide breakage in the others
#include "camp/resource/platform.hpp"

namespace camp
{
namespace resources
{
  inline namespace v1
  {
    class Event;

    namespace detail
    {
      struct EventProxyBase {
      };

      struct EventVTable {
        std::type_info const* type;
        Platform (*platform)(void const*) noexcept;
        bool (*equal)(void const*, void const*) noexcept;
        std::size_t (*hash)(void const*) noexcept;
        bool (*check)(void const*);
        void (*wait)(void const*);
      };

      template <typename T>
      inline constexpr EventVTable event_vtable_for{
          &typeid(T),
          [](void const* p) noexcept {
            static_assert(noexcept(std::declval<T const&>().get_platform()),
                          "Concrete event get_platform() must be noexcept");
            return static_cast<T const*>(p)->get_platform();
          },
          [](void const* lhs, void const* rhs) noexcept {
            static_assert(noexcept(std::declval<T const&>()
                                   == std::declval<T const&>()),
                          "Concrete event equality must be noexcept");
            return *static_cast<T const*>(lhs) == *static_cast<T const*>(rhs);
          },
          [](void const* p) noexcept {
            static_assert(noexcept(std::declval<T const&>().get_hash()),
                          "Concrete event get_hash() must be noexcept");
            return static_cast<T const*>(p)->get_hash();
          },
          [](void const* p) { return static_cast<T const*>(p)->check(); },
          [](void const* p) { static_cast<T const*>(p)->wait(); }};

      /** \brief Implementation of mutable and const non-owning event views. */
      template <bool IsConst>
      struct BasicEventRef {
        using pointer = std::conditional_t<IsConst, void const*, void*>;

        BasicEventRef() = default;

        template <camp::concepts::ConcreteEvent T>
        BasicEventRef(T& value) noexcept
            : m_event(std::addressof(value)), m_vtable(&event_vtable_for<T>)
        {
        }

        template <camp::concepts::ConcreteEvent T>
        BasicEventRef(T const& value) noexcept
          requires(IsConst)
            : m_event(std::addressof(value)), m_vtable(&event_vtable_for<T>)
        {
        }

        template <camp::concepts::ConcreteEvent T>
        BasicEventRef(T const&)
          requires(!IsConst)
        = delete;

        template <camp::concepts::ConcreteEvent T>
        BasicEventRef(T&&)
          requires std::is_rvalue_reference_v<T&&>
        = delete;

        BasicEventRef(Event& event) noexcept;
        BasicEventRef(Event const& event) noexcept
          requires(IsConst);
        BasicEventRef(Event&&) = delete;
        BasicEventRef(Event const&&) = delete;

        template <bool OtherConst>
        BasicEventRef(BasicEventRef<OtherConst> other) noexcept
          requires(IsConst && !OtherConst)
            : m_event(other.m_event), m_vtable(other.m_vtable)
        {
        }

        template <typename T>
        using access_type = std::conditional_t<IsConst, T const, T>;

        template <typename T>
        access_type<T>* try_get() const noexcept
        {
          if (!m_event || *m_vtable->type != typeid(T)) {
            return nullptr;
          }
          return static_cast<access_type<T>*>(m_event);
        }

        template <typename T>
        access_type<T>& get() const
        {
          auto* result = try_get<T>();
          if (result == nullptr) {
            ::camp::throw_re("Incompatible Event type get cast.");
          }
          return *result;
        }

        Platform get_platform() const noexcept
        {
          return m_event ? m_vtable->platform(m_event) : Platform::undefined;
        }

        bool check() const { return m_event ? m_vtable->check(m_event) : true; }

        void wait() const
        {
          if (m_event) {
            m_vtable->wait(m_event);
          }
        }

        explicit operator bool() const noexcept { return m_event != nullptr; }

        template <bool OtherConst>
        bool operator==(BasicEventRef<OtherConst> rhs) const noexcept
        {
          if (!m_event || !rhs.m_event) {
            return m_event == rhs.m_event;
          }
          return *m_vtable->type == *rhs.m_vtable->type
                 && m_vtable->equal(m_event, rhs.m_event);
        }

        template <camp::concepts::ConcreteEvent T>
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
        friend struct BasicEventRef;
        friend class ::camp::resources::Event;
        friend struct std::hash<BasicEventRef<IsConst>>;

        std::size_t get_hash() const noexcept
        {
          return m_event ? m_vtable->hash(m_event) : 0u;
        }

        pointer m_event = nullptr;
        EventVTable const* m_vtable = nullptr;
      };
    }  // namespace detail

    /**
     * \brief A mutable, non-owning, type-erased event view.
     *
     * EventRef borrows mutable concrete events or mutable owning Event
     * objects. It provides mutable typed access and converts implicitly to
     * EventConstRef. It cannot borrow const objects or temporaries and does
     * not extend the lifetime of its target.
     */
    using EventRef = detail::BasicEventRef<false>;

    /**
     * \brief A const, non-owning, type-erased event view.
     *
     * EventConstRef borrows mutable or const concrete events and owning Event
     * objects. It provides only const typed access. It cannot borrow
     * temporaries and does not extend the lifetime of its target.
     */
    using EventConstRef = detail::BasicEventRef<true>;

    /**
     * \brief An owning, type-erased event.
     *
     * Copies share ownership of the stored concrete event.
     */
    class Event : private EventRef
    {
      template <typename T>
      Event(std::shared_ptr<T> owner)
          : EventRef{*owner}, m_owner{std::move(owner)}
      {
      }

    public:
      Event() = default;
      Event(Event const&) = default;

      Event(Event&& other) noexcept : m_owner(std::move(other.m_owner))
      {
        auto& this_ref = static_cast<EventRef&>(*this);
        auto& other_ref = static_cast<EventRef&>(other);
        this_ref = std::exchange(other_ref, EventRef{});
      }

      Event& operator=(Event const&) = default;

      Event& operator=(Event&& other) noexcept
      {
        if (this != &other) {
          auto& this_ref = static_cast<EventRef&>(*this);
          auto& other_ref = static_cast<EventRef&>(other);
          this_ref = std::exchange(other_ref, EventRef{});
          m_owner = std::move(other.m_owner);
        }
        return *this;
      }

      template <camp::concepts::ConcreteEvent T>
      Event(T&& value)
          : Event(std::make_shared<camp::decay<T>>(std::forward<T>(value)))
      {
      }

      using EventRef::check;
      using EventRef::get_platform;
      using EventRef::operator bool;
      using EventRef::wait;

      template <typename T>
      T* try_get() noexcept
      {
        return EventRef::try_get<T>();
      }

      template <typename T>
      T const* try_get() const noexcept
      {
        return EventConstRef{*this}.template try_get<T>();
      }

      template <typename T>
      T& get() &
      {
        return EventRef::get<T>();
      }

      template <typename T>
      T const& get() const&
      {
        return EventConstRef{*this}.template get<T>();
      }

      template <typename T>
      T get() &&
      {
        return std::move(EventRef::get<T>());
      }

      friend bool operator==(Event const& lhs, Event const& rhs) noexcept
      {
        return EventConstRef{lhs} == EventConstRef{rhs};
      }

      template <bool IsConst>
      friend bool operator==(Event const& lhs,
                             detail::BasicEventRef<IsConst> rhs) noexcept
      {
        return EventConstRef{lhs} == rhs;
      }

      template <camp::concepts::ConcreteEvent T>
      friend bool operator==(Event const& lhs,
                             T const& rhs) noexcept(noexcept(EventConstRef{lhs}
                                                             == rhs))
      {
        return EventConstRef{lhs} == rhs;
      }

    private:
      template <bool>
      friend struct detail::BasicEventRef;
      friend struct std::hash<camp::resources::Event>;

      std::size_t get_hash() const noexcept
      {
        return EventConstRef{*this}.get_hash();
      }

      std::shared_ptr<void> m_owner;
    };

    template <bool IsConst>
    inline detail::BasicEventRef<IsConst>::BasicEventRef(Event& event) noexcept
        : BasicEventRef(static_cast<EventRef const&>(event))
    {
    }

    template <bool IsConst>
    inline detail::BasicEventRef<IsConst>::BasicEventRef(
        Event const& event) noexcept
      requires(IsConst)
        : BasicEventRef(EventConstRef{static_cast<EventRef const&>(event)})
    {
    }

  }  // namespace v1
}  // namespace resources
}  // namespace camp

namespace std
{

/** \brief Hash support for mutable event views. */
template <>
struct hash<camp::resources::EventRef> {
  std::size_t operator()(camp::resources::EventRef event) const noexcept
  {
    return event.get_hash();
  }
};

/** \brief Hash support for const event views. */
template <>
struct hash<camp::resources::EventConstRef> {
  std::size_t operator()(camp::resources::EventConstRef event) const noexcept
  {
    return event.get_hash();
  }
};

/** \brief Hash support for owning events. */
template <>
struct hash<camp::resources::Event> {
  std::size_t operator()(camp::resources::Event const& event) const noexcept
  {
    return event.get_hash();
  }
};

}  // namespace std

#endif /* __CAMP_EVENT_HPP */
