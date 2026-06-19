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

#include <memory>
#include <type_traits>

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
    namespace detail
    {
      struct EventProxyBase {
      };  // helper to identify EventProxy in sfinae
    }  // namespace detail

    class Event
    {
    public:
      Event() = default;
      Event(Event const &e) = default;
      Event(Event &&e) = default;
      Event &operator=(Event const &e) = default;
      Event &operator=(Event &&e) = default;

      template <camp::concepts::ConcreteEvent T>
      Event(T &&value)
      {
        m_value.reset(new EventModel<type::ref::rem<T>>(std::forward<T>(value)));
      }

      template <typename T>
      T* try_get()
      {
        if (!m_value) {
          return nullptr;
        }
        auto result = dynamic_cast<EventModel<T> *>(m_value.get());
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
        auto result = dynamic_cast<EventModel<T> *>(m_value.get());
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
          ::camp::throw_re("Incompatible Event type get cast.");
        }
        return *result;
      }

      template <typename T>
      T const& get() const&
      {
        T const* result = try_get<T>();
        if (result == nullptr) {
          ::camp::throw_re("Incompatible Event type get cast.");
        }
        return *result;
      }

      template <typename T>
      T get() &&
      {
        T* result = try_get<T>();
        if (result == nullptr) {
          ::camp::throw_re("Incompatible Event type get cast.");
        }
        return std::move(*result);
      }

      Platform get_platform() const
      {
        return m_value ? m_value->get_platform() : Platform::undefined;
      }

      bool check() const { return m_value ? m_value->check() : true; }

      void wait() const { if (m_value) { m_value->wait(); } }

      explicit operator bool() const { return static_cast<bool>(m_value); }

      /*
       * \brief Compares two Events to see if they represent the same underlying
       *        typed event. Two Events are equal if they have the platform and
       *        equivalent typed events.
       *
       * \return True if they have the same platform and equivalent underlying
       *         typed events, false otherwise.
       */
      friend inline bool operator==(Event const &lhs, Event const &rhs)
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

      template < camp::concepts::ConcreteEvent Evt >
      friend inline bool operator==(Event const &lhs, Evt const &rhs)
      {
        if (!lhs.m_value) {
          return false;
        }
        if (lhs.get_platform() == rhs.get_platform()) {
          if (auto lhs_event = lhs.try_get<Evt>()) {
            return *lhs_event == rhs;
          }
        }
        return false;
      }

    private:
      friend struct std::hash<camp::resources::Event>;

      /*
       * \brief Retrieves the a hash for this Event.
       * The hash allows Events to be used as keys in data structures
       * like unordered maps.
       *
       * \return A size_t hash value for this Event's
       * platform and stream/queue combination.
       *
       */
      size_t get_hash() const { return m_value ? m_value->get_hash() : 0u; }

      class EventInterface
      {
      public:
        virtual ~EventInterface() {}

        virtual Platform get_platform() const = 0;

        virtual bool compare(Event const &e) const = 0;
        virtual size_t get_hash() const = 0;

        virtual bool check() const = 0;
        virtual void wait() const = 0;
      };

      template <typename T>
      class EventModel final : public EventInterface
      {
      public:
        explicit EventModel(T modelVal)
          : m_modelVal(std::move(modelVal))
        {}

        Platform get_platform() const override
        {
          return m_modelVal.get_platform();
        }

        bool compare(Event const &e) const override
        {
          return m_modelVal == e.get<T>();
        }

        size_t get_hash() const override { return m_modelVal.get_hash(); }

        bool check() const override
        {
          return m_modelVal.check();
        }

        void wait() const override
        {
          m_modelVal.wait();
        }

        T const* get() const { return &m_modelVal; }

        T* get() { return &m_modelVal; }

      private:
        T m_modelVal;
      };

      std::shared_ptr<EventInterface> m_value;
    };

  }  // namespace v1
}  // namespace resources
}  // namespace camp

namespace std
{

/*
 * \brief Specialization of std::hash for camp::resources::Event
 *
 * Provides a hash function for Event objects, enabling their use as keys
 * in unordered associative containers (std::unordered_map, std::unordered_set,
 * etc.)
 *
 * \return A size_t hash value
 */
template <>
struct hash<camp::resources::Event> {
  std::size_t operator()(const camp::resources::Event &e) const
  {
    return e.get_hash();
  }
};

}  // namespace std

#endif /* __CAMP_EVENT_HPP */
