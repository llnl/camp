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
#include <new>
#include <optional>
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

  bool test(std::memory_order order = std::memory_order_seq_cst) noexcept
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

template <typename T>
class optional_singleton
{
public:
  // Helpers
  constexpr T& value() &
  {
    return m_data.value();
  }

  constexpr const T& value() const &
  {
    return m_data.value();
  }

  constexpr T&& value() &&
  {
    return m_data.value();
  }

  constexpr const T&& value() const &&
  {
    return m_data.value();
  }

  constexpr bool has_value() const noexcept
  {
    return m_data.has_value();
  }

  template <typename U = std::remove_cv_t<T>>
  constexpr T value_or(U&& default_value) const&
  {
    return m_data.value_or(std::forward<U>(default_value));
  }

  template <typename U = std::remove_cv_t<T>>
  constexpr T value_or(U&& default_value) &&
  {
    return m_data.value_or(std::forward<U>(default_value));
  }

  // Operators
  constexpr const T* operator->() const noexcept
  {
    return (m_data.has_value()) ? &m_data.value() : nullptr;
  }

  constexpr T* operator->() noexcept
  {
    return (m_data.has_value()) ? &m_data.value() : nullptr;
  }

  constexpr T& operator*() &
  {
    return *m_data;
  }

  constexpr const T& operator*() const &
  {
    return *m_data;
  }

  constexpr T&& operator*() &&
  {
    return *m_data;
  }

  constexpr const T&& operator*() const &&
  {
    return *m_data;
  }

  constexpr explicit operator bool() const noexcept
  {
    return static_cast<bool>(m_data);
  }

  // Modifiers
  template <typename... Args>
  requires std::constructible_from<T, Args...>
  constexpr T& emplace_once(Args&&... args)
  {
    camp::call_once(m_flag, [this, ...captured_args = std::forward<Args>(args)] () mutable {
      m_data.emplace(std::forward<Args>(captured_args)...);
    });
    return m_data.value();
  }

  void reset() noexcept
  {
    m_data.reset();
    m_flag.clear(); 
  }

private:
  std::optional<T> m_data;
  camp::resettable_once_flag m_flag;
};

}  // namespace camp

#endif // CAMP_INIT_HELPERS_HPP
