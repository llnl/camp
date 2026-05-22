##############################################################################
# Copyright (c) 2018-25, Lawrence Livermore National Security, LLC
# and Camp project contributors. See the camp/LICENSE file for details.
#
# SPDX-License-Identifier: (BSD-3-Clause)
###############################################################################

include(GoogleTest)

function(camp_add_test TESTNAME)
  cmake_parse_arguments(ABT "GTEST;RUN;OFFLOAD" "" "BUILD;TEST;DEPENDS_ON" ${ARGN})


  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TESTNAME}.cpp")
    list(APPEND ABT_BUILD "${CMAKE_CURRENT_SOURCE_DIR}/${TESTNAME}.cpp")
    list(REMOVE_DUPLICATES ABT_BUILD)
  endif()


  if(NOT CMAKE_BUILD_TYPE)
    set(TEST_BUILD_TYPE Release)
  else()
    set(TEST_BUILD_TYPE ${CMAKE_BUILD_TYPE})
  endif()

  set(_depends camp gtest)

  if(ENABLE_CUDA)
    list(APPEND _depends cuda)
  endif()

  if(ENABLE_HIP)
    list(APPEND _depends blt::hip)
  endif()

  if(ABT_DEPENDS_ON)
    list(APPEND _depends ${ABT_DEPENDS_ON})
  endif()
  # allow masking of offload tests in CI
  if(ABT_OFFLOAD)
    set(TESTNAME ${TESTNAME}.offload)
  endif(ABT_OFFLOAD)


  # Actual build setup
  blt_add_executable(
    NAME ${TESTNAME}
    SOURCES ${ABT_BUILD}
    DEPENDS_ON ${_depends}
    FOLDER test)

  # This teaches ctest about each gtest so that ctest filtering can be used
  # in azure-pipelines.yml
  gtest_discover_tests(${TESTNAME}
                       WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/test)
endfunction()

camp_add_test(array GTEST)
camp_add_test(errors GTEST)
camp_add_test(resource GTEST OFFLOAD)
camp_add_test(tuple GTEST)

camp_add_test(accumulate)
camp_add_test(append)
camp_add_test(at_key)
camp_add_test(extend)
camp_add_test(filter)
camp_add_test(find_if)
camp_add_test(flatten)
camp_add_test(index_of)
camp_add_test(lambda)
camp_add_test(number)
camp_add_test(prepend)
camp_add_test(size)
camp_add_test(transform)
