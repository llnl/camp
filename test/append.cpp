//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
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
