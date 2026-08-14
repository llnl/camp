//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef CAMP_NUMBER_NUMBER_HPP
#define CAMP_NUMBER_NUMBER_HPP

#include "camp/defines.hpp"

namespace camp
{

// TODO: document, consider making use/match std::integral_constant
template <class NumT, NumT v>
struct integral_constant {
  static constexpr NumT value = v;
  using value_type = NumT;
  using type = integral_constant;

  constexpr operator value_type() const noexcept { return value; }

  constexpr value_type operator()() const noexcept { return value; }
};

/**
 * @brief class that represents a compile time constant value.
 *
 * Some arithmetic operators are supported for this class so compile-time
 * arithmetic may be done transparently. In addition, this class is implicitly
 * convertible to its value type so it is usable in arithmetic with other types.
 */
template < auto t_value >
struct constant
{
  using type = constant;
  using value_type = decltype(t_value);
  static constexpr value_type value = t_value;

  CAMP_HOST_DEVICE
  constexpr operator value_type() const noexcept { return value; }

  CAMP_HOST_DEVICE
  constexpr value_type operator()() const noexcept { return value; }
};

/**
 * @brief constant yielding unary plus for a constant value
 */
template < auto value >
CAMP_HOST_DEVICE
constexpr auto operator+(constant<value>)
{
  return constant<+value>{};
}

/**
 * @brief constant yielding unary minus for a constant value
 */
template < auto value >
CAMP_HOST_DEVICE
constexpr auto operator-(constant<value>)
{
  return constant<-value>{};
}

/**
 * @brief constant yielding bitwise not for a constant value
 */
template < auto value >
CAMP_HOST_DEVICE
constexpr auto operator~(constant<value>)
{
  return constant<~value>{};
}

/**
 * @brief constant yielding logical not for a constant value
 */
template < auto value >
CAMP_HOST_DEVICE
constexpr auto operator!(constant<value>)
{
  return constant<!value>{};
}

/**
 * @brief constant yielding plus for constant values
 */
template < auto lhs_value, auto rhs_value >
CAMP_HOST_DEVICE
constexpr auto operator+(constant<lhs_value>, constant<rhs_value>)
{
  return constant<lhs_value+rhs_value>{};
}

/**
 * @brief constant yielding minus for constant values
 */
template < auto lhs_value, auto rhs_value >
CAMP_HOST_DEVICE
constexpr auto operator-(constant<lhs_value>, constant<rhs_value>)
{
  return constant<lhs_value-rhs_value>{};
}

/**
 * @brief constant yielding times for constant values
 */
template < auto lhs_value, auto rhs_value >
CAMP_HOST_DEVICE
constexpr auto operator*(constant<lhs_value>, constant<rhs_value>)
{
  return constant<lhs_value*rhs_value>{};
}

/**
 * @brief constant yielding divide for constant values
 */
template < auto lhs_value, auto rhs_value >
CAMP_HOST_DEVICE
constexpr auto operator/(constant<lhs_value>, constant<rhs_value>)
{
  return constant<lhs_value/rhs_value>{};
}

/**
 * @brief constant yielding remainder for constant values
 */
template < auto lhs_value, auto rhs_value >
CAMP_HOST_DEVICE
constexpr auto operator%(constant<lhs_value>, constant<rhs_value>)
{
  return constant<lhs_value%rhs_value>{};
}

/**
 * @brief constant yielding bitwise and for constant values
 */
template < auto lhs_value, auto rhs_value >
CAMP_HOST_DEVICE
constexpr auto operator&(constant<lhs_value>, constant<rhs_value>)
{
  return constant<lhs_value&rhs_value>{};
}

/**
 * @brief constant yielding bitwise or for constant values
 */
template < auto lhs_value, auto rhs_value >
CAMP_HOST_DEVICE
constexpr auto operator|(constant<lhs_value>, constant<rhs_value>)
{
  return constant<lhs_value|rhs_value>{};
}

/**
 * @brief constant yielding bitwise xor for constant values
 */
template < auto lhs_value, auto rhs_value >
CAMP_HOST_DEVICE
constexpr auto operator^(constant<lhs_value>, constant<rhs_value>)
{
  return constant<lhs_value^rhs_value>{};
}

/**
 * @brief constant yielding left shift for constant values
 */
template < auto lhs_value, auto rhs_value >
CAMP_HOST_DEVICE
constexpr auto operator<<(constant<lhs_value>, constant<rhs_value>)
{
  return constant<(lhs_value<<rhs_value)>{};
}

/**
 * @brief constant yielding right shift for constant values
 */
template < auto lhs_value, auto rhs_value >
CAMP_HOST_DEVICE
constexpr auto operator>>(constant<lhs_value>, constant<rhs_value>)
{
  return constant<(lhs_value>>rhs_value)>{};
}

/**
 * @brief constant yielding logical and for constant values
 */
template < auto lhs_value, auto rhs_value >
CAMP_HOST_DEVICE
constexpr auto operator&&(constant<lhs_value>, constant<rhs_value>)
{
  return constant<lhs_value&&rhs_value>{};
}

/**
 * @brief constant yielding logical or for constant values
 */
template < auto lhs_value, auto rhs_value >
CAMP_HOST_DEVICE
constexpr auto operator||(constant<lhs_value>, constant<rhs_value>)
{
  return constant<lhs_value||rhs_value>{};
}

/**
 * @brief constant yielding equals for constant values
 */
template < auto lhs_value, auto rhs_value >
CAMP_HOST_DEVICE
constexpr auto operator==(constant<lhs_value>, constant<rhs_value>)
{
  return constant<lhs_value==rhs_value>{};
}

/**
 * @brief constant yielding not equals for constant values
 */
template < auto lhs_value, auto rhs_value >
CAMP_HOST_DEVICE
constexpr auto operator!=(constant<lhs_value>, constant<rhs_value>)
{
  return constant<lhs_value!=rhs_value>{};
}

/**
 * @brief constant yielding less than for constant values
 */
template < auto lhs_value, auto rhs_value >
CAMP_HOST_DEVICE
constexpr auto operator<(constant<lhs_value>, constant<rhs_value>)
{
  return constant<(lhs_value<rhs_value)>{};
}

/**
 * @brief constant yielding less than equals for constant values
 */
template < auto lhs_value, auto rhs_value >
CAMP_HOST_DEVICE
constexpr auto operator<=(constant<lhs_value>, constant<rhs_value>)
{
  return constant<(lhs_value<=rhs_value)>{};
}

/**
 * @brief constant yielding greater than for constant values
 */
template < auto lhs_value, auto rhs_value >
CAMP_HOST_DEVICE
constexpr auto operator>(constant<lhs_value>, constant<rhs_value>)
{
  return constant<(lhs_value>rhs_value)>{};
}

/**
 * @brief constant yielding greater than equals for constant values
 */
template < auto lhs_value, auto rhs_value >
CAMP_HOST_DEVICE
constexpr auto operator>=(constant<lhs_value>, constant<rhs_value>)
{
  return constant<(lhs_value>=rhs_value)>{};
}


/**
 * @brief Short-form for a whole number
 *
 * @tparam N The integral value
 */
template <idx_t N>
using num = integral_constant<idx_t, N>;

using true_type = num<true>;
using false_type = num<false>;

using t = num<true>;

}  // end namespace camp

#endif /* CAMP_NUMBER_NUMBER_HPP */
