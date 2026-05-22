//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2018-26, Lawrence Livermore National Security, LLC
// and Camp project contributors. See the camp/LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include <camp/camp.hpp>

using namespace camp;

using list1 = list<float, double, double>;
using list2 = list<int, int, char>;

CAMP_CHECK_TSAME((extend<list1, list2>), (list<float, double, double, int, int, char>));
CAMP_CHECK_TSAME((extend<list2, list1>), (list<int, int, char, float, double, double>));
CAMP_CHECK_TSAME((extend<list<int>, list<int>>), (list<int, int>));
CAMP_CHECK_TSAME((extend<list<char, float>, list<>>), (list<char, float>));
CAMP_CHECK_TSAME((extend<list<>, list<char, float>>), (list<char, float>));
CAMP_CHECK_TSAME((extend<list<>, list1>), (list1));
CAMP_CHECK_TSAME((extend<list2, list<>>), (list2));
