###############################################################################
# Copyright (c) Lawrence Livermore National Security, LLC and other
# Camp Project Developers. See top-level LICENSE and COPYRIGHT
# files for dates and other details. No copyright assignment is required
# to contribute to Camp.
#
# SPDX-License-Identifier: (BSD-3-Clause)
###############################################################################

cmake_minimum_required (VERSION 3.10)

project (camp
  LANGUAGES CXX C
  VERSION 2025.12.0)

set(camp_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(camp_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(camp_VERSION_PATCH ${PROJECT_VERSION_PATCH})

include(CheckCXXCompilerFlag)
if(NOT DEFINED BLT_CXX_STD)
  set(CXX_VERSIONS 20)
  foreach(cxxver ${CXX_VERSIONS})
    if("cxx_std_${cxxver}" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
      if (ENABLE_CUDA AND (NOT DEFINED CMAKE_CUDA_COMPILE_FEATURES OR (NOT "cuda_std_${cxxver}" IN_LIST CMAKE_CUDA_COMPILE_FEATURES)))
        continue()
      endif()
      set(CAMP_CXX_STD ${cxxver})
      break()
    endif()
  endforeach()
  if(NOT DEFINED CAMP_CXX_STD)
    set(CAMP_CXX_STD 20)
  endif()
  set(BLT_CXX_STD c++${CAMP_CXX_STD} CACHE STRING "Version of C++
  standard")
  message("Using C++ standard: ${BLT_CXX_STD}")
else() #check BLT_CXX_STD is high enough by disallowing the only invalid option
  set(_unsupported_cxx "c++98" "c++11" "c++14" "c++17")
  if (BLT_CXX_STD IN_LIST _unsupported_cxx)
    message(FATAL_ERROR "CAMP and the RAJA framework no
    longer support ${_unsupported_cxx}, select a c++
    standard of 20 or higher")
  endif()
endif(NOT DEFINED BLT_CXX_STD)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/load_blt.cmake)


cmake_dependent_option(CAMP_ENABLE_DOCUMENTATION "Build Docs" Off "ENABLE_DOCS" Off)
cmake_dependent_option(CAMP_ENABLE_TESTS "Build tests" On "ENABLE_TESTS" Off)

# if ENABLE_TESTS is defined by a parent project, and
# CAMP_ENABLE_TESTS has not been set to OFF set the
# value of CAMP_ENABLE_TESTS to the value of ENABLE_TESTS.
if (CAMP_ENABLE_TESTS AND DEFINED ENABLE_TESTS)
  set(CAMP_ENABLE_TESTS ${ENABLE_TESTS})
endif()

if (WIN32)
  # use git-bash for windows, wsl is not populated on azure
  set(BASH "C:/Program Files/Git/bin/bash.exe")
else()
  set(BASH "bash")
endif()
# generate list of headers at configure time, no this is not perfect
# note that this *DOES* work on windows if bash is installed, which
# it is on azure
execute_process(COMMAND ${BASH} ${PROJECT_SOURCE_DIR}/scripts/gen-header-list.sh
  OUTPUT_FILE ${PROJECT_BINARY_DIR}/camp_headers.cmake
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
include(${PROJECT_BINARY_DIR}/camp_headers.cmake)
list (APPEND camp_headers ${PROJECT_BINARY_DIR}/include/camp/config.hpp)

# backends
set (camp_depends)

set (camp_backends openmp cuda hip sycl)
# NOTE: camp itself doesn't require a hip or cuda compiler,
#       but some of its features are activated by having a device compiler
set (camp_runtime_backends cuda hip)

foreach (backend ${camp_backends})
  string(TOUPPER "${backend}" suffix)
  # NOTE: can't do this because of the lack of sycl support and usage upstream
  # in blt
  # cmake_dependent_option("CAMP_ENABLE_${suffix}" "Enable ${backend}
  #   backend" On "ENABLE_${suffix}" Off)
  if ("${ENABLE_${suffix}}")
    set ("CAMP_ENABLE_${suffix}" On)
  endif()
  if (${CAMP_ENABLE_${suffix}})
    if (backend IN_LIST camp_runtime_backends)
      set (backend ${backend}_runtime)
    endif()
    if (TARGET blt::${backend})
      set (backend blt::${backend})
    endif()
    list (APPEND camp_depends ${backend})
  endif()
endforeach()

cmake_dependent_option(CAMP_ENABLE_TARGET_OPENMP "Enable OpenMP offload as
device runtime" Off "CAMP_ENABLE_OPENMP" Off)

if (ENABLE_CUDA)
  if("${CUDA_VERSION_STRING}" VERSION_LESS "10.1")
    message(FATAL_ERROR "Trying to use CUDA version ${CUDA_VERSION_STRING}. CAMP requires CUDA version 10.1 or newer.")
  endif()
endif ()

if (ENABLE_HIP)
  if("${hip_VERSION}" VERSION_LESS "3.5")
    message(FATAL_ERROR "Trying to use HIP/ROCm version ${hip_VERSION}. CAMP requires HIP/ROCm version 3.5 or newer. ")
  endif()
endif ()

# end backends

option(CAMP_USE_PLATFORM_DEFAULT_STREAM "Whether CAMP should use the default device streams (On) or create separate streams (Off)" Off)

# Configure the config header file to allow config time options
configure_file(${PROJECT_SOURCE_DIR}/include/camp/config.in.hpp
  ${PROJECT_BINARY_DIR}/include/camp/config.hpp)

blt_add_library (
  NAME camp
  HEADERS ${camp_headers}
  SOURCES ./src/errors.cpp
  DEPENDS_ON ${camp_depends}
  )
if(cxx_std_${CAMP_CXX_STD} IN_LIST CMAKE_CXX_COMPILE_FEATURES)
  target_compile_features(camp PUBLIC cxx_std_${CAMP_CXX_STD})
endif()

if (COMPILER_FAMILY_IS_MSVC)
  if (NOT BUILD_SHARED_LIBS)
    target_compile_definitions(camp PUBLIC CAMP_WIN_STATIC_BUILD)
  else (NOT BUILD_SHARED_LIBS)
    target_compile_definitions(camp PRIVATE CAMP_DLL_EXPORTS)
  endif (NOT BUILD_SHARED_LIBS)
endif ()

target_include_directories (camp PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
  )
set_target_properties (camp PROPERTIES
  INTERFACE_LIB_VERSION ${PROJECT_VERSION}
  INTERFACE_COMPILE_FEATURES cxx_std_14)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${PROJECT_BINARY_DIR}/campConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
  )

install(TARGETS
  camp
  EXPORT campTargets
  RUNTIME DESTINATION lib
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/campConfig.cmake.in"
  "${PROJECT_BINARY_DIR}/campConfig.cmake"
  INSTALL_DESTINATION
  lib/cmake/camp
  )

install(EXPORT campTargets
  DESTINATION lib/cmake/camp)
install(FILES
  "${PROJECT_BINARY_DIR}/campConfigVersion.cmake"
  "${PROJECT_BINARY_DIR}/campConfig.cmake"
  DESTINATION
  lib/cmake/camp)
install(DIRECTORY
  ${PROJECT_SOURCE_DIR}/include/
  DESTINATION
  include)
install(FILES
  "${PROJECT_BINARY_DIR}/include/camp/config.hpp"
  DESTINATION
  include/camp)

if(CAMP_ENABLE_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

if(CAMP_ENABLE_DOCUMENTATION)
  add_subdirectory(docs)
endif()

