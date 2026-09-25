//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include <thread>

#include "camp/camp.hpp"
#include "camp/init_helpers.hpp"
#include "gtest/gtest.h"

namespace detail
{
  struct MyStruct
  {
    int my_int{5};
  };
};

TEST(CampInitHelpers, SimpleCallOnce)
{
  int test = 0;
  camp::resettable_once_flag flag;

  camp::call_once(flag, [&] () { test += 1; });
  ASSERT_EQ(test, 1);

  camp::call_once(flag, [&] () { test += 1; });
  ASSERT_EQ(test, 1);

  flag.clear();
  camp::call_once(flag, [&] () { test += 1; });
  ASSERT_EQ(test, 2);
}

TEST(CampInitHelpers, ThreadedCallOnce)
{
  int test = 0;
  camp::resettable_once_flag flag;

  auto add_num_once = [&] (int i) {
    camp::call_once(flag, [&test] (int j) { 
      // Ensures operation won't be too quick to test contention for
      // lock  
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      test += j;
    }, i); 
  };

  std::thread t1(add_num_once, 1);
  std::thread t2(add_num_once, 1);
  std::thread t3(add_num_once, 1);
  std::thread t4(add_num_once, 1);
  t1.join();
  t2.join();
  t3.join();
  t4.join();

  ASSERT_EQ(test, 1);

  flag.clear();
  const int num = 5;
  std::thread t5(add_num_once, num);
  std::thread t6(add_num_once, num);
  std::thread t7(add_num_once, num);
  std::thread t8(add_num_once, num);
  t5.join();
  t6.join();
  t7.join();
  t8.join();
  ASSERT_EQ(test, num+1);
}

TEST(CampResettableSingleton, Construct)
{
  camp::resettable_singleton<int> test;
  CAMP_ALLOW_UNUSED_LOCAL(test);
}

TEST(CampResettableSingleton, HasValue)
{
  camp::resettable_singleton<int> test;
  ASSERT_EQ(test.has_value(), false);
}

TEST(CampResettableSingleton, EmplaceOnce)
{
  camp::resettable_singleton<int> test;
  ASSERT_EQ(test.has_value(), false);

  test.emplace_once(5);
  ASSERT_EQ(test.has_value(), true);
  ASSERT_EQ(test.value(), 5);
  ASSERT_EQ(*test, 5);
  ASSERT_EQ(test.get_or_emplace(), 5);

  test.emplace_once(10);
  ASSERT_EQ(test.has_value(), true);
  ASSERT_EQ(test.value(), 5);
  ASSERT_EQ(*test, 5);
}

TEST(CampResettableSingleton, ThreadedEmplaceOnce)
{
  camp::resettable_singleton<int> test;
  ASSERT_EQ(test.has_value(), false);

  auto set_once = [&] (int i) {
    test.emplace_once(i);
  };

  const int num = 5;
  std::thread t1(set_once, num);
  std::thread t2(set_once, num);
  std::thread t3(set_once, num);
  std::thread t4(set_once, num);
  t1.join();
  t2.join();
  t3.join();
  t4.join();
  ASSERT_EQ(test.has_value(), true);
  ASSERT_EQ(test.value(), num);
  ASSERT_EQ(*test, num);
  ASSERT_EQ(test.get_or_emplace(), num);

  const int num2 = 10;
  std::thread t5(set_once, num2);
  std::thread t6(set_once, num2);
  std::thread t7(set_once, num2);
  std::thread t8(set_once, num2);
  t5.join();
  t6.join();
  t7.join();
  t8.join();
  ASSERT_EQ(test.has_value(), true);
  ASSERT_EQ(test.value(), num); // still should be original number
  ASSERT_EQ(*test, num); // still should be original number
  ASSERT_EQ(test.get_or_emplace(), num); // still should be original number
}

TEST(CampResettableSingleton, GetOrEmplace)
{
  camp::resettable_singleton<int> test;
  ASSERT_EQ(test.has_value(), false);

  const int val = test.get_or_emplace(5);
  ASSERT_EQ(test.has_value(), true);
  ASSERT_EQ(test.value(), 5);
  ASSERT_EQ(val, 5);
  ASSERT_EQ(*test, 5);
  ASSERT_EQ(test.get_or_emplace(), 5);

  const int val2 = test.get_or_emplace(10);
  ASSERT_EQ(test.has_value(), true);
  ASSERT_EQ(test.value(), 5);
  ASSERT_EQ(val2, 5);
  ASSERT_EQ(*test, 5);
  ASSERT_EQ(test.get_or_emplace(), 5);
}

TEST(CampResettableSingleton, ThreadedGetOrEmplace)
{
  camp::resettable_singleton<int> test;
  ASSERT_EQ(test.has_value(), false);

  auto set_once = [&] (int i) {
    test.get_or_emplace(i);
  };

  const int num = 5;
  std::thread t1(set_once, num);
  std::thread t2(set_once, num);
  std::thread t3(set_once, num);
  std::thread t4(set_once, num);
  t1.join();
  t2.join();
  t3.join();
  t4.join();
  ASSERT_EQ(test.has_value(), true);
  ASSERT_EQ(test.value(), num);
  ASSERT_EQ(*test, num);
  ASSERT_EQ(test.get_or_emplace(), num);

  const int num2 = 10;
  std::thread t5(set_once, num2);
  std::thread t6(set_once, num2);
  std::thread t7(set_once, num2);
  std::thread t8(set_once, num2);
  t5.join();
  t6.join();
  t7.join();
  t8.join();
  ASSERT_EQ(test.has_value(), true);
  ASSERT_EQ(test.value(), num); // still should be original number
  ASSERT_EQ(*test, num); // still should be original number
  ASSERT_EQ(test.get_or_emplace(), num); // still should be original number
}

TEST(CampResettableSingleton, OperatorMemberAccess)
{
  camp::resettable_singleton<detail::MyStruct> test;
  ASSERT_EQ(test.has_value(), false);

  const auto& val = test.get_or_emplace();
  ASSERT_EQ(test.has_value(), true);
  ASSERT_EQ(test.value().my_int, 5);
  ASSERT_EQ(val.my_int, 5);
  ASSERT_EQ(test.get_or_emplace().my_int, 5);
  ASSERT_EQ(test->my_int, 5);

  const auto& val2 = test.get_or_emplace();
  ASSERT_EQ(test.has_value(), true);
  ASSERT_EQ(test.value().my_int, 5);
  ASSERT_EQ(val2.my_int, 5);
  ASSERT_EQ(test.get_or_emplace().my_int, 5);
  ASSERT_EQ(test->my_int, 5);
}

TEST(CampResettableSingleton, Reset)
{
  camp::resettable_singleton<int> test;
  ASSERT_EQ(test.has_value(), false);

  test.emplace_once(5);
  ASSERT_EQ(test.has_value(), true);
  ASSERT_EQ(test.value(), 5);
  ASSERT_EQ(*test, 5);
  ASSERT_EQ(test.get_or_emplace(), 5);

  test.reset();
  ASSERT_EQ(test.has_value(), false);

  test.emplace_once(10);
  ASSERT_EQ(test.has_value(), true);
  ASSERT_EQ(test.value(), 10);
  ASSERT_EQ(*test, 10);
  ASSERT_EQ(test.get_or_emplace(), 10);
}

TEST(CampResettableSingleton, ResetGetOrEmplace)
{
  camp::resettable_singleton<int> test;
  ASSERT_EQ(test.has_value(), false);

  const int val = test.get_or_emplace(5);
  ASSERT_EQ(test.has_value(), true);
  ASSERT_EQ(test.value(), 5);
  ASSERT_EQ(val, 5);
  ASSERT_EQ(*test, 5);
  ASSERT_EQ(test.get_or_emplace(), 5);

  test.reset();
  ASSERT_EQ(test.has_value(), false);

  const int val2 = test.get_or_emplace(10);
  ASSERT_EQ(test.has_value(), true);
  ASSERT_EQ(test.value(), 10);
  ASSERT_EQ(val2, 10);
  ASSERT_EQ(*test, 10);
  ASSERT_EQ(test.get_or_emplace(), 10);
}

TEST(CampResettableSingleton, ThreadedReset)
{
  camp::resettable_singleton<int> test;
  ASSERT_EQ(test.has_value(), false);

  auto set_once = [&] (int i) {
    test.emplace_once(i);
  };

  const int num = 5;
  std::thread t1(set_once, num);
  std::thread t2(set_once, num);
  std::thread t3(set_once, num);
  std::thread t4(set_once, num);
  t1.join();
  t2.join();
  t3.join();
  t4.join();
  ASSERT_EQ(test.has_value(), true);
  ASSERT_EQ(test.value(), num);
  ASSERT_EQ(*test, num);
  ASSERT_EQ(test.get_or_emplace(), num);

  test.reset();
  ASSERT_EQ(test.has_value(), false);

  const int num2 = 10;
  std::thread t5(set_once, num2);
  std::thread t6(set_once, num2);
  std::thread t7(set_once, num2);
  std::thread t8(set_once, num2);
  t5.join();
  t6.join();
  t7.join();
  t8.join();
  ASSERT_EQ(test.has_value(), true);
  ASSERT_EQ(test.value(), num2);
  ASSERT_EQ(*test, num2);
  ASSERT_EQ(test.get_or_emplace(), num2);
}
