//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef CAMP_INIT_HELPERS_HPP
#define CAMP_INIT_HELPERS_HPP

#include <atomic>
#include <concepts>
#include <mutex>
#include <memory>
#include <new>
#include <utility>

namespace camp
{
/// Resettable version of std::once_flag
///
/// This is similar to std::once_flag used in std::call_once. However,
/// this version supports resetting flag, so that memory can be cleaned
/// and re-initialized later if needed.
/// 
/// @note Currently, this uses atomic<bool> instead of atomic_flag to support
///       older versions of GCC. atomic_flag::test is not supported until
///       GCC 11. atomic_flag would be preferred as it is guaranteed to
///       not use a lock.
class resettable_once_flag
{
public:
  resettable_once_flag() = default; 

  resettable_once_flag(const resettable_once_flag&) = delete;
  resettable_once_flag& operator=(const resettable_once_flag&) = delete;

  resettable_once_flag(resettable_once_flag&&) = delete;
  resettable_once_flag& operator=(resettable_once_flag&&) = delete;

  bool test(std::memory_order order = std::memory_order_seq_cst) const noexcept
  {
    return m_flag.load(order);
  }

  void set(bool value, std::memory_order order = std::memory_order_seq_cst) noexcept
  {
    m_flag.store(value, order);
  }

  void clear()
  {
    m_flag.store(false, std::memory_order_release);
  }

  std::mutex& get_mutex() noexcept
  {
    return m_lock;
  }

private:
  std::mutex m_lock{};
  std::atomic<bool> m_flag{false};
};

/// Resettable version of std::call_once
///
/// This is similar to std::call_once. However, this version supports
/// uses a custom ``camp::resettable_once_flag`` that allows the
/// data to be cleaned up. If needed, calling this function again after
/// resetting the flag will call the callable function again.
template <typename Callable, typename... Args>
void call_once(camp::resettable_once_flag& flag, Callable&& callable, Args&&... args)
{
  if (flag.test(std::memory_order::acquire)) [[likely]] {
    return;
  }

  std::lock_guard guard(flag.get_mutex());
  if (!flag.test(std::memory_order::relaxed)) {
    callable(std::forward<Args>(args)...);
    flag.set(true, std::memory_order::release);
  }
}

/// Policy to determine default behavior of the optional singleton class.
/// Some types have non-trivial deconstructors that could be freed
/// after main if `reset` is not called. If cleanup
/// order after main is not clear, then it may be preferred to avoid
/// automatic cleanup, which is the None policy. The default behavior
/// will call the deconstructor.
enum class OptionalDtorPolicy
{
  Default,
  None
};

/// Resettable version of a singleton
///
/// This class aims to have clear semnatics around initialization and
/// destruction of singleton types. Types will not be constructed until
/// `emplace_once` is called. Additionally, if need be types, can be cleaned up
/// and re-initialized. If types can't be cleaned up at the end of `main`, then
/// the `OptionalDtorPolicy::None` can be used. This avoids trying to deconstruct
/// an object when the singleton goes out of scope.
///
/// \note This allows constant initialization (initialization at compiler time)
/// for all types due the construction of the \tparam{T} happening at a later time.
/// 
template <typename T, OptionalDtorPolicy Policy = OptionalDtorPolicy::Default>
class optional_singleton
{
public:
  optional_singleton() = default;

  ~optional_singleton() requires (Policy == OptionalDtorPolicy::None || std::is_trivially_destructible_v<T>) = default;

  ~optional_singleton()
  requires (Policy == OptionalDtorPolicy::Default && !std::is_trivially_destructible_v<T>)
  {
    reset();
  }

  // Helpers
  constexpr T& value() noexcept
  {
    return *std::launder(reinterpret_cast<T*>(m_storage));
  }

  constexpr const T& value() const
  {
    return *std::launder(reinterpret_cast<const T*>(m_storage));
  }

  constexpr bool has_value() const noexcept
  {
    return m_flag.test(std::memory_order_acquire);
  }

  // Operators
  constexpr explicit operator bool() const noexcept
  {
    return has_value();
  }

  // Modifiers
  template <typename... Args>
  requires std::constructible_from<T, Args...>
  constexpr T& emplace_once(Args&&... args)
  {
    camp::call_once(m_flag, [this, ...captured_args = std::forward<Args>(args)] () mutable {
      std::construct_at(reinterpret_cast<T*>(m_storage), std::forward<Args>(captured_args)...);
    });
    return value();
  }

  void reset() noexcept
  {
    if (has_value()) { std::destroy_at(reinterpret_cast<T*>(m_storage)); }
    m_flag.clear(); 
  }

private:
  alignas(alignof(T)) unsigned char m_storage[sizeof(T)]{};
  camp::resettable_once_flag m_flag;
};

}  // namespace camp

#endif // CAMP_INIT_HELPERS_HPP
