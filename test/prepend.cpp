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

CAMP_CHECK_TSAME((prepend<list1, list2>), (list<list<int, int, char>, float, double, double>));
CAMP_CHECK_TSAME((prepend<list2, list1>), (list<list<float, double, double>, int, int, char>));

CAMP_CHECK_TSAME((prepend<list<int>, int>), (list<int, int>));
CAMP_CHECK_TSAME((prepend<list1, int>), (list<int, float, double, double>));
CAMP_CHECK_TSAME((prepend<list<>, int>), (list<int>));
CAMP_CHECK_TSAME((prepend<list<>, list<char, int>>), (list<list<char, int>>));
