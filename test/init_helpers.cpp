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
