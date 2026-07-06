//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include "camp/resource.hpp"

#include <numeric>
#include <vector>

#include "camp/camp.hpp"
#include "gtest/gtest.h"

using namespace camp::resources;

template <typename Res>
void test_construct()
{
  camp::ResourceAllocator<int, Res> alloc1{Res()};
  camp::ResourceAllocator<int, Res> alloc2{Res(), MemoryAccess::Pinned};
  camp::ResourceAllocator<int, Res> alloc3{Res(), MemoryAccess::Device};
  camp::ResourceAllocator<int, Res> alloc4{Res(), MemoryAccess::Managed};
  CAMP_ALLOW_UNUSED_LOCAL(alloc1);
  CAMP_ALLOW_UNUSED_LOCAL(alloc2);
  CAMP_ALLOW_UNUSED_LOCAL(alloc3);
  CAMP_ALLOW_UNUSED_LOCAL(alloc4);
}

//
TEST(CampResourceAllocator, Construct)
{
  test_construct<Host>();
#ifdef CAMP_HAVE_CUDA
  test_construct<Cuda>();
#endif
#ifdef CAMP_HAVE_HIP
  test_construct<Hip>();
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_construct<Omp>();
#endif
#ifdef CAMP_HAVE_SYCL
  test_construct<Sycl>();
#endif
}

template <typename Res>
void test_copy()
{
  camp::ResourceAllocator<int, Res> alloc1{Res()};
  auto alloc2 = alloc1;
  camp::ResourceAllocator<int, Res> alloc3 = alloc1;
  CAMP_ALLOW_UNUSED_LOCAL(alloc2);
  CAMP_ALLOW_UNUSED_LOCAL(alloc3);
}

//
TEST(CampResourceAllocator, Copy)
{
  test_copy<Host>();
#ifdef CAMP_HAVE_CUDA
  test_copy<Cuda>();
#endif
#ifdef CAMP_HAVE_HIP
  test_copy<Hip>();
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_copy<Omp>();
#endif
#ifdef CAMP_HAVE_SYCL
  test_copy<Sycl>();
#endif
}

template <typename Res>
void test_copy_assignment()
{
  camp::ResourceAllocator<int, Res> src{Res()};
  camp::ResourceAllocator<int, Res> dst;

  dst = src;

  ASSERT_EQ(src, dst);
  ASSERT_EQ(src.get_resource(), dst.get_resource());
  ASSERT_EQ(src.get_mem_access(), dst.get_mem_access());
}

TEST(CampResourceAllocator, CopyAssignment)
{
  test_copy_assignment<Host>();
#ifdef CAMP_HAVE_CUDA
  test_copy_assignment<Cuda>();
#endif
#ifdef CAMP_HAVE_HIP
  test_copy_assignment<Hip>();
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_copy_assignment<Omp>();
#endif
#ifdef CAMP_HAVE_SYCL
  test_copy_assignment<Sycl>();
#endif
}

template <typename Res>
void test_move_assignment()
{
  camp::ResourceAllocator<int, Res> src{Res()};
  camp::ResourceAllocator<int, Res> dst;

  dst = std::move(src);
}

TEST(CampResourceAllocator, MoveAssignment)
{
  test_move_assignment<Host>();
#ifdef CAMP_HAVE_CUDA
  test_move_assignment<Cuda>();
#endif
#ifdef CAMP_HAVE_HIP
  test_move_assignment<Hip>();
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_move_assignment<Omp>();
#endif
#ifdef CAMP_HAVE_SYCL
  test_move_assignment<Sycl>();
#endif
}

template <typename Res>
void test_get_resource(Res res)
{
  camp::ResourceAllocator<int, Res> alloc1{res};
  const camp::ResourceAllocator<int, Res> const_alloc1{res};

  ASSERT_EQ(alloc1.get_resource(), res);
  ASSERT_EQ(const_alloc1.get_resource(), res);
  ASSERT_EQ(alloc1.get_resource(), const_alloc1.get_resource());
}

TEST(CampResourceAllocator, GetResource)
{
  test_get_resource(Host{});
#ifdef CAMP_HAVE_CUDA
  test_get_resource(Cuda{});
#endif
#ifdef CAMP_HAVE_HIP
  test_get_resource(Hip{});
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_get_resource(Omp{});
#endif
#ifdef CAMP_HAVE_SYCL
  test_get_resource(Sycl{});
#endif
}

template <typename Res>
void test_get_mem_access()
{
  camp::ResourceAllocator<int, Res> alloc1{Res(), MemoryAccess::Pinned};
  camp::ResourceAllocator<int, Res> alloc2{Res(), MemoryAccess::Device};
  camp::ResourceAllocator<int, Res> alloc3{Res(), MemoryAccess::Managed};

  ASSERT_EQ(alloc1.get_mem_access(), alloc1.get_mem_access());
  ASSERT_NE(alloc1.get_mem_access(), alloc2.get_mem_access());
  ASSERT_NE(alloc1.get_mem_access(), alloc3.get_mem_access());

  ASSERT_EQ(alloc2.get_mem_access(), alloc2.get_mem_access());
  ASSERT_NE(alloc2.get_mem_access(), alloc3.get_mem_access());

  ASSERT_EQ(alloc3.get_mem_access(), alloc3.get_mem_access());
}

TEST(CampResourceAllocator, GetMemAccess)
{
  test_get_mem_access<Host>();
#ifdef CAMP_HAVE_CUDA
  test_get_mem_access<Cuda>();
#endif
#ifdef CAMP_HAVE_HIP
  test_get_mem_access<Hip>();
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_get_mem_access<Omp>();
#endif
#ifdef CAMP_HAVE_SYCL
  test_get_mem_access<Sycl>();
#endif
}

template <typename Res>
void test_compare()
{
  camp::ResourceAllocator<int, Res> alloc1{Res(), MemoryAccess::Pinned};
  camp::ResourceAllocator<int, Res> alloc2{Res(), MemoryAccess::Device};
  camp::ResourceAllocator<int, Res> alloc3{Res(), MemoryAccess::Managed};

  ASSERT_EQ(alloc1, alloc1);
  ASSERT_NE(alloc1, alloc2);
  ASSERT_NE(alloc1, alloc3);
}

//
TEST(CampResourceAllocator, Compare) 
{
  test_compare<Host>();
#ifdef CAMP_HAVE_CUDA
  test_compare<Cuda>();
#endif
#ifdef CAMP_HAVE_HIP
  test_compare<Hip>();
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_compare<Omp>();
#endif
#ifdef CAMP_HAVE_SYCL
  test_compare<Sycl>();
#endif
}

template <typename Res>
void test_rebind()
{
  camp::ResourceAllocator<int, Res> alloc1{Res()};
  camp::ResourceAllocator<double, Res> alloc2{alloc1};

  ASSERT_EQ(alloc1, alloc2);
  ASSERT_EQ(alloc1.get_resource(), alloc2.get_resource());
  ASSERT_EQ(alloc1.get_mem_access(), alloc2.get_mem_access());
}

//
TEST(CampResourceAllocator, Rebind)
{
  test_rebind<Host>();
#ifdef CAMP_HAVE_CUDA
  test_rebind<Cuda>();
#endif
#ifdef CAMP_HAVE_HIP
  test_rebind<Hip>();
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_rebind<Omp>();
#endif
#ifdef CAMP_HAVE_SYCL
  test_rebind<Sycl>();
#endif
}

template <typename Res>
void test_allocate()
{
  static constexpr std::size_t num = 5;
  camp::ResourceAllocator<int, Res> alloc1{Res()};

  int* ptr = alloc1.allocate(num);

  EXPECT_TRUE(ptr != nullptr);

  alloc1.deallocate(ptr, num);
}

//
TEST(CampResourceAllocator, Allocate)
{
  test_allocate<Host>();
#ifdef CAMP_HAVE_CUDA
  test_allocate<Cuda>();
#endif
#ifdef CAMP_HAVE_HIP
  test_allocate<Hip>();
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_allocate<Omp>();
#endif
#ifdef CAMP_HAVE_SYCL
  test_allocate<Sycl>();
#endif
}

template <typename Res>
void test_vector()
{
  static constexpr std::size_t num = 5;
  camp::ResourceAllocator<int, Res> alloc{Res(), MemoryAccess::Pinned};

  std::vector vec(num, alloc);
  std::iota(vec.begin(), vec.end(), num);

  EXPECT_TRUE(vec.data() != nullptr);
  EXPECT_TRUE(vec.size() == num);

  for (std::size_t i = 0; i < vec.size(); ++i) {
    EXPECT_TRUE(vec[i] == static_cast<int>(i));
  }
}

//
TEST(CampResourceAllocator, Vector)
{
  test_allocate<Host>();
#ifdef CAMP_HAVE_CUDA
  test_allocate<Cuda>();
#endif
#ifdef CAMP_HAVE_HIP
  test_allocate<Hip>();
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_allocate<Omp>();
#endif
#ifdef CAMP_HAVE_SYCL
  test_allocate<Sycl>();
#endif
}
