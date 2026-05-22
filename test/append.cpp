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

CAMP_CHECK_TSAME((append<list1, list2>), (list<float, double, double, list<int, int, char>>));
CAMP_CHECK_TSAME((append<list2, list1>), (list<int, int, char, list<float, double, double>>));

CAMP_CHECK_TSAME((append<list<int>, int>), (list<int, int>));
CAMP_CHECK_TSAME((append<list1, int>), (list<float, double, double, int>));
CAMP_CHECK_TSAME((append<list<>, int>), (list<int>));
CAMP_CHECK_TSAME((append<list<>, list<char, int>>), (list<list<char, int>>));
