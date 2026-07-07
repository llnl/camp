//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include "camp/array.hpp"
#include "camp/tuple.hpp"

#include "Test.hpp"
#include "gtest/gtest.h"

CAMP_TEST_BEGIN(array, initialize)
{
  camp::array<int, 3> a{1, 2, 10};

  return a[0] == 1 && a[1] == 2 && a[2] == 10;
}
CAMP_TEST_END(array, initialize)

CAMP_TEST_BEGIN(array, copy_initialize)
{
  camp::array<int, 3> a = {10, 2, 1};

  return a[0] == 10 && a[1] == 2 && a[2] == 1;
}
CAMP_TEST_END(array, copy_initialize)

CAMP_TEST_BEGIN(array, copy_construct)
{
  camp::array<int, 3> a{1, 2, 10};
  camp::array<int, 3> b{a};
  a[1] = 1;

  return b[0] == 1 && b[1] == 2 && b[2] == 10;
}
CAMP_TEST_END(array, copy_construct)

CAMP_TEST_BEGIN(array, copy_assignment)
{
  camp::array<int, 3> a{1, 2, 10};
  camp::array<int, 3> b{3, 4, 6};
  a = b;
  b[1] = 1;

  return a[0] == 3 && a[1] == 4 && a[2] == 6;
}
CAMP_TEST_END(array, copy_assignment)

// Not portable as currently implemented
TEST(host_array, at)
{
  camp::array<int, 2> a = {-4, 4};
  const camp::array<int, 2>& b = a;

  a.at(1) = 8;

  int resultAt0 = -1;
  int resultAt1 = -1;
  int resultAt2 = -1;
  bool exception = false;

  try {
    resultAt0 = a.at(0);
    resultAt1 = b.at(1);
    resultAt2 = a.at(2);
  } catch (const std::out_of_range&) {
    exception = true;
  }

  EXPECT_EQ(resultAt0, -4);
  EXPECT_EQ(resultAt1, 8);
  EXPECT_EQ(resultAt2, -1);
  EXPECT_TRUE(exception);
}

CAMP_TEST_BEGIN(array, subscript)
{
  camp::array<int, 2> a = {1, 8};

  bool passed = a[0] == 1 && a[1] == 8;

  a[0] = 3;

  return passed && a[0] == 3 && a[1] == 8;
}

CAMP_TEST_END(array, subscript);

CAMP_TEST_BEGIN(array, front)
{
  camp::array<int, 2> a = {1, 8};

  bool passed = a.front() == 1 && a[0] == 1 && a[1] == 8;

  a.front() = 3;

  return passed && a.front() == 3 && a[0] == 3 && a[1] == 8;
}
CAMP_TEST_END(array, front)

CAMP_TEST_BEGIN(array, back)
{
  camp::array<int, 2> a = {1, 8};

  bool passed = a[0] == 1 && a[1] == 8 && a.back() == 8;

  a.back() = 3;

  return passed && a[0] == 1 && a[1] == 3 && a.back() == 3;
}
CAMP_TEST_END(array, back)

CAMP_TEST_BEGIN(array, data)
{
  camp::array<int, 2> a = {1, 8};
  int* a_data = a.data();

  const camp::array<int, 2>& b{a};
  const int* b_data = b.data();

  bool passed =
      a_data[0] == 1 && a_data[1] == 8 && b_data[0] == 1 && b_data[1] == 8;

  a_data[0] = 3;

  return passed && a_data[0] == 3 && a_data[1] == 8 && b_data[0] == 3
         && b_data[1] == 8;
}
CAMP_TEST_END(array, data)

CAMP_TEST_BEGIN(array, begin)
{
  camp::array<int, 2> a = {1, 8};
  auto a_it = a.begin();

  const camp::array<int, 2>& b{a};
  auto b_it = b.begin();

  bool passed = *a_it++ == 1 && *a_it++ == 8 && *b_it++ == 1 && *b_it++ == 8;

  a_it = a.begin();
  *a_it = 4;
  b_it = b.begin();

  return passed && *a_it++ == 4 && *a_it++ == 8 && *b_it++ == 4 && *b_it++ == 8;
}
CAMP_TEST_END(array, begin)

CAMP_TEST_BEGIN(array, cbegin)
{
  camp::array<int, 2> a = {1, 8};
  auto a_it = a.cbegin();

  const camp::array<int, 2>& b{a};
  auto b_it = b.cbegin();

  return *(a_it++) == 1 && *(a_it++) == 8 && *(b_it++) == 1 && *(b_it++) == 8;
}
CAMP_TEST_END(array, cbegin)

CAMP_TEST_BEGIN(array, end)
{
  camp::array<int, 2> a = {1, 8};
  auto a_it = a.end();

  const camp::array<int, 2>& b{a};
  auto b_it = b.end();

  bool passed =
      *(--a_it) == 8 && *(--a_it) == 1 && *(--b_it) == 8 && *(--b_it) == 1;

  a_it = a.end();
  *(--a_it) = 4;

  a_it = a.end();
  b_it = b.end();

  return passed && *(--a_it) == 4 && *(--a_it) == 1 && *(--b_it) == 4
         && *(--b_it) == 1;
}
CAMP_TEST_END(array, end)

CAMP_TEST_BEGIN(array, cend)
{
  camp::array<int, 2> a = {1, 8};
  auto a_it = a.cend();
  --a_it;

  const camp::array<int, 2>& b{a};
  auto b_it = b.cend();

  return *(a_it--) == 8 && *(a_it--) == 1 && *(--b_it) == 8 && *(--b_it) == 1;
}
CAMP_TEST_END(array, cend)

CAMP_TEST_BEGIN(array, empty)
{
  // Zero sized arrays are technically not allowed,
  // and are explicitly disallowed in device code.
  camp::array<double, 1> a{1.0};

  return !a.empty();
}
CAMP_TEST_END(array, empty)

CAMP_TEST_BEGIN(array, size)
{
  // Zero sized arrays are technically not allowed,
  // and are explicitly disallowed in device code.
  camp::array<double, 2> a{1.0, 3.0};

  return a.size() == 2;
}
CAMP_TEST_END(array, size)

CAMP_TEST_BEGIN(array, max_size)
{
  // Zero sized arrays are technically not allowed,
  // and are explicitly disallowed in device code.
  camp::array<double, 2> a{1.0, 3.0};

  return a.size() == 2;
}
CAMP_TEST_END(array, max_size)

CAMP_TEST_BEGIN(array, fill)
{
  camp::array<int, 3> a{1, 2, 3};
  a.fill(0);

  return a[0] == 0 && a[1] == 0 && a[2] == 0;
}
CAMP_TEST_END(array, fill)

CAMP_TEST_BEGIN(array, swap)
{
  camp::array<int, 3> a{1, 2, 3};
  camp::array<int, 3> b{3, 2, 1};
  camp::swap(a, b);  // Calls a.swap(b), so covers that as well

  return a[0] == 3 && a[1] == 2 && a[2] == 1 && b[0] == 1 && b[1] == 2
         && b[2] == 3;
}
CAMP_TEST_END(array, swap)

CAMP_TEST_BEGIN(array, equal)
{
  camp::array<int, 2> a{1, 2};
  camp::array<int, 2> b{1, 3};

  return a == a && !(a == b);
}
CAMP_TEST_END(array, equal)

CAMP_TEST_BEGIN(array, not_equal)
{
  camp::array<int, 2> a{1, 2};
  camp::array<int, 2> b{1, 3};

  return a != b && !(a != a);
}
CAMP_TEST_END(array, not_equal)

CAMP_TEST_BEGIN(array, less_than)
{
  camp::array<int, 2> a{1, 2};
  camp::array<int, 2> b{1, 3};

  return !(a < a) && a < b && !(b < a);
}
CAMP_TEST_END(array, less_than)

CAMP_TEST_BEGIN(array, less_than_or_equal)
{
  camp::array<int, 2> a{1, 2};
  camp::array<int, 2> b{1, 3};

  return a <= a && a <= b && !(b <= a);
}
CAMP_TEST_END(array, less_than_or_equal)

CAMP_TEST_BEGIN(array, greater_than)
{
  camp::array<int, 2> a{1, 2};
  camp::array<int, 2> b{1, 3};

  return !(a > a) && b > a && !(a > b);
}
CAMP_TEST_END(array, greater_than)

CAMP_TEST_BEGIN(array, greater_than_or_equal)
{
  camp::array<int, 2> a{1, 2};
  camp::array<int, 2> b{1, 3};

  return a >= a && b >= a && !(a >= b);
}
CAMP_TEST_END(array, greater_than_or_equal)

CAMP_TEST_BEGIN(array, get_lvalue_reference)
{
  camp::array<int, 2> a = {1, 8};
  const camp::array<int, 2>& b{a};

  bool passed = camp::get<0>(a) == 1 && camp::get<1>(a) == 8
                && camp::get<0>(b) == 1 && camp::get<1>(b) == 8;

  camp::get<0>(a) = 3;

  return passed && camp::get<0>(a) == 3 && camp::get<1>(a) == 8
         && camp::get<0>(b) == 3 && camp::get<1>(b) == 8;
}
CAMP_TEST_END(array, get_lvalue_reference)

// TODO: Write tests involving types with move constructors
CAMP_TEST_BEGIN(array, get_rvalue_reference)
{
  camp::array<int, 2> a = {1, 8};
  int&& a0 = camp::get<0>(camp::move(a));

  const camp::array<int, 2> b{6, 8};
  const int&& b1 = camp::get<1>(camp::move(b));

  return a0 == 1 && b1 == 8;
}
CAMP_TEST_END(array, get_rvalue_reference)

CAMP_TEST_BEGIN(array, to_array)
{
  int temp[3] = {1, 2, 10};
  camp::array<int, 3> a = camp::to_array(temp);
  camp::array<int, 3> b = camp::to_array(camp::move(temp));

  return a[0] == 1 && a[1] == 2 && a[2] == 10 && b[0] == 1 && b[1] == 2
         && b[2] == 10;
}
CAMP_TEST_END(array, to_array)

CAMP_TEST_BEGIN(array, tuple_size)
{
  constexpr std::size_t size7 = std::tuple_size<camp::array<double, 7>>::value;
  constexpr std::size_t size11 =
      std::tuple_size<camp::array<double, 11>>::value;

  return size7 == 7 && size11 == 11;
}
CAMP_TEST_END(array, tuple_size)

CAMP_TEST_BEGIN(array, tuple_element)
{
  constexpr bool element0 =
      std::is_same<double,
                   std::tuple_element_t<0, camp::array<double, 5>>>::value;
  constexpr bool element4 =
      std::is_same<double,
                   std::tuple_element_t<4, camp::array<double, 5>>>::value;

  return element0 && element4;
}
CAMP_TEST_END(array, tuple_element)

#if defined(__cplusplus) && __cplusplus >= 201703L

CAMP_TEST_BEGIN(array, apply)
{
  camp::array<int, 2> a{-1, 1};
  int a0 = 0;
  int a1 = 0;
  camp::apply([&](int i0, int i1){
    a0 = i0;
    a1 = i1;
  }, a);

  return a0 == -1 && a1 == 1 && a[0] == -1 && a[1] == 1;
}
CAMP_TEST_END(array, apply)

CAMP_TEST_BEGIN(array, structured_binding)
{
  camp::array<int, 2> a{-1, 1};
  auto& [a0, a1] = a;
  a1 = 3;

  return a0 == -1 && a1 == 3 && a[0] == -1 && a[1] == 3;
}
CAMP_TEST_END(array, structured_binding)

// If CUDA is enabled and building with c++17, must use up-to-date CUDA version
// Or this test will fail. Worked with cuda/11.8.0 and clang/18.1.8
CAMP_TEST_BEGIN(array, deduction_guide)
{
  camp::array a{-1, 1};

  return a[0] == -1 && a[1] == 1;
}
CAMP_TEST_END(array, deduction_guide)

#endif
