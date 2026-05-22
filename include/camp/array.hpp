//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// The implementation of camp::array follows the C++ standard but borrows
// from the implementation of std::array in the LLVM project at:
// https://github.com/llvm/llvm-project/blob/main/libcxx/include/array
//
// The license information from that file is included below.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef camp_array_HPP__
#define camp_array_HPP__

#include <cstddef>
#include <stdexcept>
#include <type_traits>

#include "camp/defines.hpp"
#include "camp/helpers.hpp"
#include "camp/number.hpp"

namespace camp
{
///
/// Provides a portable std::array-like class.
///
/// Differences from std::array are listed below:
/// - No __device__ "at" method (exceptions are not GPU friendly)
/// - Calling front or back is a compile error when size() == 0
///   (instead of undefined behavior at run time)
/// - No reverse iterators have been implemented yet
/// - Swap is yet to be implemented
///
template <class T, std::size_t N>
struct array {
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using iterator = pointer;
  using const_iterator = const_pointer;

  // TODO: Investigate device trap
  ///
  /// Accesses the element at the specified index with bounds checking.
  ///
  /// @param i The index of the element to access.
  /// @return A reference to the element at the specified index.
  /// @throws std::out_of_range if the index is out of bounds.
  constexpr reference at(size_type i)
  {
    if (i >= N) {
      throw std::out_of_range{"camp::array::at detected out of range access"};
    }

    return elements[i];
  }

  ///
  /// Accesses the element at the specified index with bounds checking (const
  /// version).
  ///
  /// @param i The index of the element to access.
  /// @return A const reference to the element at the specified index.
  /// @throws std::out_of_range if the index is out of bounds.
  constexpr const_reference at(size_type i) const
  {
    if (i >= N) {
      throw std::out_of_range{"camp::array::at detected out of range access"};
    }

    return elements[i];
  }

  CAMP_HOST_DEVICE constexpr reference operator[](size_type i) noexcept
  {
    return elements[i];
  }

  CAMP_HOST_DEVICE constexpr const_reference operator[](
      size_type i) const noexcept
  {
    return elements[i];
  }

  CAMP_HOST_DEVICE constexpr reference front() noexcept
  {
    static_assert(N > 0,
                  "Calling camp::array::front on an empty array is not "
                  "allowed.");
    return elements[0];
  }

  CAMP_HOST_DEVICE constexpr const_reference front() const noexcept
  {
    static_assert(N > 0,
                  "Calling camp::array::front on an empty array is not "
                  "allowed.");
    return elements[0];
  }

  CAMP_HOST_DEVICE constexpr reference back() noexcept
  {
    static_assert(N > 0,
                  "Calling camp::array::back on an empty array is not "
                  "allowed.");
    return elements[N - 1];
  }

  CAMP_HOST_DEVICE constexpr const_reference back() const noexcept
  {
    static_assert(N > 0,
                  "Calling camp::array::back on an empty array is not "
                  "allowed.");
    return elements[N - 1];
  }

  CAMP_HOST_DEVICE constexpr pointer data() noexcept { return elements; }

  CAMP_HOST_DEVICE constexpr const_pointer data() const noexcept
  {
    return elements;
  }

  CAMP_HOST_DEVICE constexpr iterator begin() noexcept
  {
    return iterator(elements);
  }

  CAMP_HOST_DEVICE constexpr const_iterator begin() const noexcept
  {
    return const_iterator(elements);
  }

  CAMP_HOST_DEVICE constexpr const_iterator cbegin() const noexcept
  {
    return const_iterator(elements);
  }

  CAMP_HOST_DEVICE constexpr iterator end() noexcept
  {
    return iterator(elements + N);
  }

  CAMP_HOST_DEVICE constexpr const_iterator end() const noexcept
  {
    return const_iterator(elements + N);
  }

  CAMP_HOST_DEVICE constexpr const_iterator cend() const noexcept
  {
    return const_iterator(elements + N);
  }

  CAMP_HOST_DEVICE constexpr bool empty() const noexcept { return N == 0; }

  CAMP_HOST_DEVICE constexpr size_type size() const noexcept { return N; }

  CAMP_HOST_DEVICE constexpr size_type max_size() const noexcept { return N; }

  CAMP_HOST_DEVICE constexpr void fill(const T& value)
  {
    for (std::size_t i = 0; i < N; ++i) {
      elements[i] = value;
    }
  }

  // TODO: Revisit swap implementations to make them match the std library
  CAMP_HOST_DEVICE constexpr typename std::enable_if<
      std::is_nothrow_move_constructible<T>::value
      && std::is_nothrow_move_assignable<T>::value>::type
  swap(array& a) noexcept(std::is_nothrow_swappable<T>::value)
  {
    for (std::size_t i = 0; i < N; ++i) {
      safe_swap(elements[i], a[i]);
    }
  }

  value_type elements[N];
};

template <class T, std::size_t N>
CAMP_HOST_DEVICE inline constexpr bool operator==(const array<T, N>& lhs,
                                                  const array<T, N>& rhs)
{
  for (std::size_t i = 0; i < N; ++i) {
    if (!(lhs[i] == rhs[i])) {
      return false;
    }
  }

  return true;
}

template <class T, std::size_t N>
CAMP_HOST_DEVICE inline constexpr bool operator!=(const array<T, N>& lhs,
                                                  const array<T, N>& rhs)
{
  return !(lhs == rhs);
}

template <class T, std::size_t N>
CAMP_HOST_DEVICE inline constexpr bool operator<(const array<T, N>& lhs,
                                                 const array<T, N>& rhs)
{
  for (std::size_t i = 0; i < N; ++i) {
    if (lhs[i] < rhs[i]) {
      return true;
    } else if (rhs[i] < lhs[i]) {
      return false;
    }
  }

  return false;
}

template <class T, std::size_t N>
CAMP_HOST_DEVICE inline constexpr bool operator<=(const array<T, N>& lhs,
                                                  const array<T, N>& rhs)
{
  return !(rhs < lhs);
}

template <class T, std::size_t N>
CAMP_HOST_DEVICE inline constexpr bool operator>(const array<T, N>& lhs,
                                                 const array<T, N>& rhs)
{
  return rhs < lhs;
}

template <class T, std::size_t N>
CAMP_HOST_DEVICE inline constexpr bool operator>=(const array<T, N>& lhs,
                                                  const array<T, N>& rhs)
{
  return !(lhs < rhs);
}

template <std::size_t I, class T, std::size_t N>
CAMP_HOST_DEVICE inline constexpr T& get(array<T, N>& a) noexcept
{
  static_assert(I < N, "Index out of bounds in camp::get<> (camp::array&)");
  return a[I];
}

template <std::size_t I, class T, std::size_t N>
CAMP_HOST_DEVICE inline constexpr T&& get(array<T, N>&& a) noexcept
{
  static_assert(I < N, "Index out of bounds in camp::get<> (camp::array&&)");
  return move(a[I]);
}

template <std::size_t I, class T, std::size_t N>
CAMP_HOST_DEVICE inline constexpr const T& get(const array<T, N>& a) noexcept
{
  static_assert(I < N,
                "Index out of bounds in camp::get<> (const camp::array&)");
  return a[I];
}

template <std::size_t I, class T, std::size_t N>
CAMP_HOST_DEVICE inline constexpr const T&& get(const array<T, N>&& a) noexcept
{
  static_assert(I < N,
                "Index out of bounds in camp::get<> (const camp::array&&)");
  return move(a[I]);
}

template <class T, size_t N>
CAMP_HOST_DEVICE constexpr void swap(array<T, N>& a, array<T, N>& b) noexcept(
    noexcept(a.swap(b)))
{
  a.swap(b);
}

namespace detail
{
  template <class T, std::size_t N, std::size_t... I>
  CAMP_HOST_DEVICE inline constexpr array<std::remove_cv_t<T>, N> to_array_impl(
      T (&a)[N],
      int_seq<std::size_t, I...>)
  {
    return {{a[I]...}};
  }

  template <class T, std::size_t N, std::size_t... I>
  CAMP_HOST_DEVICE inline constexpr array<std::remove_cv_t<T>, N> to_array_impl(
      T (&&a)[N],
      int_seq<std::size_t, I...>)
  {
    return {{move(a[I])...}};
  }
}  // namespace detail

template <class T, std::size_t N>
CAMP_HOST_DEVICE inline constexpr array<std::remove_cv_t<T>, N> to_array(
    T (&a)[N])
{
  return detail::to_array_impl(a, make_int_seq_t<std::size_t, N>{});
}

template <class T, std::size_t N>
CAMP_HOST_DEVICE inline constexpr array<std::remove_cv_t<T>, N> to_array(
    T (&&a)[N])
{
  return detail::to_array_impl(move(a), make_int_seq_t<std::size_t, N>{});
}

#if defined(__cplusplus) && __cplusplus >= 201703L
///
/// Deduction guide
///
/// TODO: Find a way to make sure all U's are the same:
///       https://github.com/llvm/llvm-project/blob/main/libcxx/include/array#L383
template <class T, class... U>
CAMP_HOST_DEVICE array(T, U...) -> array<T, 1 + sizeof...(U)>;
#endif
}  // namespace camp

// For structured bindings
namespace std
{
template <class T, std::size_t N>
struct tuple_size<camp::array<T, N>>
    : public std::integral_constant<std::size_t, N> {
};

template <std::size_t I, class T, std::size_t N>
struct tuple_element<I, camp::array<T, N>> {
  static_assert(I < N,
                "Index out of bounds in std::tuple_element<> (camp::array)");
  using type = T;
};
}  // namespace std

#endif  // !defined(CAMP_ARRAY_H)
