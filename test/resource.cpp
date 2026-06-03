//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include "camp/resource.hpp"

#include <array>
#include <type_traits>

#include "camp/camp.hpp"
#include "gtest/gtest.h"

using namespace camp::resources;

// compatible but different resource for conversion test
struct Host2 : Host {
};
struct HostEvent2 : HostEvent {
};
#ifdef CAMP_HAVE_CUDA
struct Cuda2 : Cuda {
};
#endif
#ifdef CAMP_HAVE_HIP
struct Hip2 : Hip {
};
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
struct Omp2 : Omp {
};
#endif
#ifdef CAMP_HAVE_SYCL
struct Sycl2 : Sycl {
};
#endif

namespace camp
{
namespace resources
{
  template <>
  struct is_concrete_resource_impl<Host2> : std::true_type {
  };
  template <>
  struct is_concrete_event_impl<HostEvent2> : std::true_type {
  };
}  // namespace resources
}  // namespace camp

struct NotAResource { };

struct NotAnEvent { };

template <typename Res>
void test_construct()
{
  Resource r{Res()};
  CAMP_ALLOW_UNUSED_LOCAL(r);
}

//
TEST(CampResource, Construct)
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
  Resource r1{Res()};
  auto r2 = r1;
  Resource r3 = r1;
  CAMP_ALLOW_UNUSED_LOCAL(r2);
  CAMP_ALLOW_UNUSED_LOCAL(r3);
}

//
TEST(CampResource, Copy)
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

template <typename Res, typename Res2>
void test_convert_fails()
{
  Resource r{Res()};
  r.get<Res>();
  ASSERT_THROW(r.get<Res2>(), std::runtime_error);
  ASSERT_FALSE(r.try_get<Res2>());
}

//
TEST(CampResource, ConvertFails)
{
  test_convert_fails<Host, Host2>();
#ifdef CAMP_HAVE_CUDA
  test_convert_fails<Cuda, Cuda2>();
#endif
#ifdef CAMP_HAVE_HIP
  test_convert_fails<Hip, Hip2>();
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_convert_fails<Omp, Omp2>();
#endif
#ifdef CAMP_HAVE_SYCL
  test_convert_fails<Sycl, Sycl2>();
#endif
}

template <typename Res>
void test_convert_works(Platform platform)
{
  Resource r{Res()};
  ASSERT_TRUE(r.try_get<Res>());
  ASSERT_EQ(r.get<Res>().get_platform(), platform);
}

//
TEST(CampResource, ConvertWorks)
{
  test_convert_works<Host>(Platform::host);
#ifdef CAMP_HAVE_CUDA
  test_convert_works<Cuda>(Platform::cuda);
#endif
#ifdef CAMP_HAVE_HIP
  test_convert_works<Hip>(Platform::hip);
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_convert_works<Omp>(Platform::omp_target);
#endif
#ifdef CAMP_HAVE_SYCL
  test_convert_works<Sycl>(Platform::sycl);
#endif
}

TEST(CampResource, GetPlatform)
{
  ASSERT_EQ(static_cast<const Resource>(Host()).get_platform(), Platform::host);
#ifdef CAMP_HAVE_CUDA
  ASSERT_EQ(static_cast<const Resource>(Cuda()).get_platform(), Platform::cuda);
#endif
#ifdef CAMP_HAVE_HIP
  ASSERT_EQ(static_cast<const Resource>(Hip()).get_platform(), Platform::hip);
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  ASSERT_EQ(static_cast<const Resource>(Omp()).get_platform(),
            Platform::omp_target);
#endif
#ifdef CAMP_HAVE_SYCL
  ASSERT_EQ(static_cast<const Resource>(Sycl()).get_platform(), Platform::sycl);
#endif
}

TEST(CampEvent, GetPlatform)
{
  ASSERT_EQ(static_cast<const Event>(Host().get_event_erased()).get_platform(),
            Platform::host);
#ifdef CAMP_HAVE_CUDA
  ASSERT_EQ(static_cast<const Event>(Cuda().get_event_erased()).get_platform(),
            Platform::cuda);
#endif
#ifdef CAMP_HAVE_HIP
  ASSERT_EQ(static_cast<const Event>(Hip().get_event_erased()).get_platform(),
            Platform::hip);
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  ASSERT_EQ(static_cast<const Event>(Omp().get_event_erased()).get_platform(),
            Platform::omp_target);
#endif
#ifdef CAMP_HAVE_SYCL
  ASSERT_EQ(static_cast<const Event>(Sycl().get_event_erased()).get_platform(),
            Platform::sycl);
#endif
}

TEST(CampResource, EmptyBehavior)
{
  Resource full{Host()};
  Resource sink{std::move(full)};
  Resource full2{Host()};
  Resource sink2{std::move(full2)};
  Resource& empty = full;
  Resource& empty2 = full2;
  int value = 7;

  CAMP_ALLOW_UNUSED_LOCAL(sink);
  CAMP_ALLOW_UNUSED_LOCAL(sink2);

  ASSERT_FALSE(empty);
  ASSERT_EQ(empty.get_platform(), Platform::undefined);
  ASSERT_EQ(empty, empty2);
  ASSERT_NE(empty, Resource{Host()});
  ASSERT_EQ(std::hash<Resource>{}(empty), 0u);

  ASSERT_EQ(empty.allocate<int>(0), nullptr);
  ASSERT_EQ(empty.calloc(0), nullptr);
  empty.deallocate(nullptr);
  empty.memcpy(&value, &value, 0);
  empty.memset(&value, 0, 0);
  empty.wait();
  empty.wait_for(nullptr);

  ASSERT_THROW((void)empty.get<Host>(), std::runtime_error);
  ASSERT_FALSE(empty.try_get<Host>());
  ASSERT_THROW((void)empty.allocate<int>(1), std::runtime_error);
  ASSERT_THROW((void)empty.calloc(1), std::runtime_error);
  ASSERT_THROW(empty.deallocate(&value), std::runtime_error);
  ASSERT_THROW(empty.memcpy(&value, &value, 1), std::runtime_error);
  ASSERT_THROW(empty.memset(&value, 0, 1), std::runtime_error);

  Event empty_event = empty.get_event();
  Event empty_erased_event = empty.get_event_erased();
  ASSERT_FALSE(empty_event);
  ASSERT_FALSE(empty_erased_event);
  ASSERT_EQ(empty_event, empty_erased_event);

  Event host_event = Host().get_event_erased();
  empty.wait_for(&host_event);
}

TEST(CampEvent, EmptyBehavior)
{
  Event empty;
  Event empty2;

  ASSERT_FALSE(empty);
  ASSERT_EQ(empty.get_platform(), Platform::undefined);
  ASSERT_TRUE(empty.check());
  empty.wait();
  ASSERT_EQ(empty, empty2);
  ASSERT_NE(empty, Host().get_event_erased());
  ASSERT_EQ(std::hash<Event>{}(empty), 0u);
  ASSERT_FALSE(empty.try_get<HostEvent>());
  ASSERT_THROW((void)empty.get<HostEvent>(), std::runtime_error);

  Event host_event{Host().get_event()};
  ASSERT_TRUE(host_event);
  ASSERT_TRUE(host_event.try_get<HostEvent>());
  ASSERT_FALSE(host_event.try_get<HostEvent2>());
  ASSERT_THROW((void)host_event.get<HostEvent2>(), std::runtime_error);
}

TEST(CampPlatform, ResourceFromPlatform)
{
  ASSERT_TRUE((std::is_same_v<resource_from_platform<Platform::host>::type,
                              Host>));
#ifdef CAMP_HAVE_CUDA
  ASSERT_TRUE((std::is_same_v<resource_from_platform<Platform::cuda>::type,
                              Cuda>));
#endif
#ifdef CAMP_HAVE_HIP
  ASSERT_TRUE((std::is_same_v<resource_from_platform<Platform::hip>::type,
                              Hip>));
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  ASSERT_TRUE(
      (std::is_same_v<resource_from_platform<Platform::omp_target>::type, Omp>));
#endif
#ifdef CAMP_HAVE_SYCL
  ASSERT_TRUE((std::is_same_v<resource_from_platform<Platform::sycl>::type,
                              Sycl>));
#endif
}

template <typename Res>
void test_zero_size_memory_ops()
{
  Resource r{Res()};
  int value = 13;

  ASSERT_EQ(r.allocate<int>(0), nullptr);
  ASSERT_EQ(r.calloc(0), nullptr);
  r.deallocate(nullptr);
  r.memcpy(&value, &value, 0);
  r.memset(&value, 0, 0);
}

TEST(CampResource, ZeroSizeMemory)
{
  test_zero_size_memory_ops<Host>();
#ifdef CAMP_HAVE_CUDA
  test_zero_size_memory_ops<Cuda>();
#endif
#ifdef CAMP_HAVE_HIP
  test_zero_size_memory_ops<Hip>();
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_zero_size_memory_ops<Omp>();
#endif
#ifdef CAMP_HAVE_SYCL
  test_zero_size_memory_ops<Sycl>();
#endif
}

template <typename Res>
void test_vector(Resource& h)
{
  // Generic
  std::vector<Resource> vec;
  Resource d1{Res()};
  Resource d2{Res()};

  // Typed
  std::vector<Res> rvec;
  Res r1;
  Res r2;

  // Generic
  vec.emplace_back(Host());
  vec.emplace_back(h);
  vec.emplace_back(d1);
  vec.emplace_back(r1);

  // Typed
  rvec.emplace_back(Res());
  rvec.emplace_back(d2.get<Res>());
  rvec.emplace_back(r2);

  // Verify using Resource in a vector works
  // Generic
  ASSERT_EQ(vec.size(), 4);
  ASSERT_EQ(vec[0], h);
  ASSERT_EQ(vec[1], h);
  ASSERT_EQ(vec[2], d1);
  ASSERT_EQ(vec[3], r1);

  // Typed
  ASSERT_EQ(rvec.size(), 3);
  if constexpr (std::same_as<Res, Host>) {
    ASSERT_EQ(rvec[0], Res());
  } else {
    ASSERT_NE(rvec[0], Res());
  }
  ASSERT_EQ(rvec[1], d2);
  ASSERT_EQ(rvec[2], r2);
}

//
TEST(CampResource, Vector)
{
  Resource h{Host()};
  test_vector<Host>(h);
#ifdef CAMP_HAVE_CUDA
  test_vector<Cuda>(h);
#endif
#ifdef CAMP_HAVE_HIP
  test_vector<Hip>(h);
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_vector<Omp>(h);
#endif
#ifdef CAMP_HAVE_SYCL
  test_vector<Sycl>(h);
#endif
}

template <typename Res>
void test_vector(Event& he)
{
  using typed_event = typename Res::event_type;

  // Generic
  Event d1 = Res().get_event_erased();
  Event d2 = Res().get_event_erased();
  std::vector<Event> vec;

  // Typed
  typed_event e1 = Res().get_event();
  typed_event e2 = Res().get_event();
  std::vector<typed_event> rvec;

  // Generic
  vec.emplace_back(Host().get_event());
  vec.emplace_back(Host().get_event_erased());
  vec.emplace_back(std::move(d1));
  vec.emplace_back(std::move(e1));
  vec.emplace_back(Res().get_event());
  vec.emplace_back(Res().get_event_erased());

  // Typed
  rvec.emplace_back(std::move(std::move(d2).get<typed_event>()));
  rvec.emplace_back(std::move(e2));
  rvec.emplace_back(Res().get_event());

  // Verify using Event in a vector works
  // Generic
  ASSERT_EQ(vec.size(), 6);
  ASSERT_EQ(vec[0], he);
  ASSERT_EQ(vec[0], vec[1]);
  if constexpr (std::same_as<Res, Host>) {
    ASSERT_EQ(vec[1], vec[2]);
    ASSERT_EQ(vec[2], vec[3]);
    ASSERT_EQ(vec[3], vec[4]);
    ASSERT_EQ(vec[4], vec[5]);
  } else {
    ASSERT_NE(vec[1], vec[2]);
    ASSERT_NE(vec[2], vec[3]);
    ASSERT_NE(vec[3], vec[4]);
    ASSERT_NE(vec[4], vec[5]);
  }

  for (Event& e : vec) {
    (void)e;
  }

  // Typed
  ASSERT_EQ(rvec.size(), 3);
  if constexpr (std::same_as<Res, Host>) {
    ASSERT_EQ(rvec[0], rvec[1]);
    ASSERT_EQ(rvec[1], rvec[2]);
  } else {
    ASSERT_NE(rvec[0], rvec[1]);
    ASSERT_NE(rvec[1], rvec[2]);
  }

  for (typed_event& e : rvec) {
    (void)e;
  }
}

//
TEST(CampEvent, Vector)
{
  Event he = Host().get_event_erased();
  test_vector<Host>(he);
#ifdef CAMP_HAVE_CUDA
  test_vector<Cuda>(he);
#endif
#ifdef CAMP_HAVE_HIP
  test_vector<Hip>(he);
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_vector<Omp>(he);
#endif
#ifdef CAMP_HAVE_SYCL
  test_vector<Sycl>(he);
#endif
}

template <typename Res>
void test_map_key(Resource& h)
{
  // Generic
  std::unordered_map<Resource, size_t> map;
  std::unordered_multimap<Resource, size_t> multimap;
  Resource d1{Res()};
  Resource d2{Res()};

  // Typed
  std::unordered_map<Res, size_t> rmap;
  std::unordered_multimap<Res, size_t> rmultimap;
  Res r1;
  Res r2;

  // Generic
  map.insert({h, 10});
  multimap.insert({h, 10});
  map.insert({h, 20});
  multimap.insert({h, 20});
  map.insert({d1, 30});
  multimap.insert({d1, 30});
  map.insert({d2, 40});
  multimap.insert({d2, 40});
  map.insert({d2, 50});
  multimap.insert({d2, 50});

  // Typed
  rmap.insert({r1, 30});
  rmultimap.insert({r1, 30});
  rmap.insert({r2, 40});
  rmultimap.insert({r2, 40});
  rmap.insert({r2, 50});
  rmultimap.insert({r2, 50});

  // Verify using Resource as a key to find entries works
  // Generic
  if constexpr (std::same_as<Res, Host>) {
    ASSERT_EQ(map.count(h), 1);
    ASSERT_EQ(multimap.count(h), 5);
    ASSERT_EQ(map.count(d1), 1);
    ASSERT_EQ(multimap.count(d1), 5);
    ASSERT_EQ(map.count(d2), 1);
    ASSERT_EQ(multimap.count(d2), 5);
  } else {
    ASSERT_EQ(map.count(h), 1);
    ASSERT_EQ(multimap.count(h), 2);
    ASSERT_EQ(map.count(d1), 1);
    ASSERT_EQ(multimap.count(d1), 1);
    ASSERT_EQ(map.count(d2), 1);
    ASSERT_EQ(multimap.count(d2), 2);
  }

  // Typed
  if constexpr (std::same_as<Res, Host>) {
    ASSERT_EQ(rmap.count(r1), 1);
    ASSERT_EQ(rmultimap.count(r1), 3);
    ASSERT_EQ(rmap.count(r2), 1);
    ASSERT_EQ(rmultimap.count(r2), 3);
  } else {
    ASSERT_EQ(rmap.count(r1), 1);
    ASSERT_EQ(rmultimap.count(r1), 1);
    ASSERT_EQ(rmap.count(r2), 1);
    ASSERT_EQ(rmultimap.count(r2), 2);
  }

  // Verify equal_range works
  // Generic
  auto range = map.equal_range(h);
  auto range2 = multimap.equal_range(d2);
  if constexpr (std::same_as<Res, Host>) {
    ASSERT_EQ(std::distance(range.first, range.second), 1);
    ASSERT_EQ(std::distance(range2.first, range2.second), 5);
  } else {
    ASSERT_EQ(std::distance(range.first, range.second), 1);
    ASSERT_EQ(std::distance(range2.first, range2.second), 2);
  }

  // Typed
  auto rrange = rmap.equal_range(r1);
  auto rrange2 = rmultimap.equal_range(r2);
  if constexpr (std::same_as<Res, Host>) {
    ASSERT_EQ(std::distance(rrange.first, rrange.second), 1);
    ASSERT_EQ(std::distance(rrange2.first, rrange2.second), 3);
  } else {
    ASSERT_EQ(std::distance(rrange.first, rrange.second), 1);
    ASSERT_EQ(std::distance(rrange2.first, rrange2.second), 2);
  }
}

//
TEST(CampResource, UnorderedMapKey)
{
  Resource h{Host()};
  test_map_key<Host>(h);
#ifdef CAMP_HAVE_CUDA
  test_map_key<Cuda>(h);
#endif
#ifdef CAMP_HAVE_HIP
  test_map_key<Hip>(h);
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_map_key<Omp>(h);
#endif
#ifdef CAMP_HAVE_SYCL
  test_map_key<Sycl>(h);
#endif
}

struct ref_hash {
  template <typename T>
  std::size_t operator()(std::reference_wrapper<T> const& ref) const {
    return std::hash<T>()(ref.get());
  }
};

struct ref_equal {
  template <typename T>
  bool operator()(std::reference_wrapper<T> const& lhs,
                  std::reference_wrapper<T> const& rhs) const {
    return lhs.get() == rhs.get();
  }
};

template <typename Res>
void test_map_key(Event& he)
{
  using typed_event = typename Res::event_type;

  using erased_event_ref = std::reference_wrapper<Event>;
  using typed_event_ref = std::reference_wrapper<typed_event>;

  // Generic
  Event d1_ = Res().get_event_erased();
  Event d2_ = Res().get_event_erased();
  erased_event_ref d1(d1_);
  erased_event_ref d2(d2_);
  std::unordered_map<erased_event_ref, size_t, ref_hash, ref_equal> map;
  std::unordered_multimap<erased_event_ref, size_t, ref_hash, ref_equal> multimap;

  // Typed
  typed_event e1_ = Res().get_event();
  typed_event e2_ = Res().get_event();
  typed_event_ref e1(e1_);
  typed_event_ref e2(e2_);
  std::unordered_map<typed_event_ref, size_t, ref_hash, ref_equal> rmap;
  std::unordered_multimap<typed_event_ref, size_t, ref_hash, ref_equal> rmultimap;

  // Generic
  map.emplace(he, 10);
  multimap.emplace(he, 10);
  map.emplace(he, 20);
  multimap.emplace(he, 20);
  map.emplace(d1, 30);
  multimap.emplace(d1, 30);
  map.emplace(d2, 40);
  multimap.emplace(d2, 40);
  map.emplace(d2, 50);
  multimap.emplace(d2, 50);

  // Typed
  rmap.emplace(e1, 30);
  rmultimap.emplace(e1, 30);
  rmap.emplace(e2, 40);
  rmultimap.emplace(e2, 40);
  rmap.emplace(e2, 50);
  rmultimap.emplace(e2, 50);

  // Verify using Event as a key to find entries works
  // Generic
  if constexpr (std::same_as<Res, Host>) {
    ASSERT_EQ(map.count(he), 1);
    ASSERT_EQ(multimap.count(he), 5);
    ASSERT_EQ(map.count(d1), 1);
    ASSERT_EQ(multimap.count(d1), 5);
    ASSERT_EQ(map.count(d2), 1);
    ASSERT_EQ(multimap.count(d2), 5);
  } else {
    ASSERT_EQ(map.count(he), 1);
    ASSERT_EQ(multimap.count(he), 2);
    ASSERT_EQ(map.count(d1), 1);
    ASSERT_EQ(multimap.count(d1), 1);
    ASSERT_EQ(map.count(d2), 1);
    ASSERT_EQ(multimap.count(d2), 2);
  }

  // Typed
  if constexpr (std::same_as<Res, Host>) {
    ASSERT_EQ(rmap.count(e1), 1);
    ASSERT_EQ(rmultimap.count(e1), 3);
    ASSERT_EQ(rmap.count(e2), 1);
    ASSERT_EQ(rmultimap.count(e2), 3);
  } else {
    ASSERT_EQ(rmap.count(e1), 1);
    ASSERT_EQ(rmultimap.count(e1), 1);
    ASSERT_EQ(rmap.count(e2), 1);
    ASSERT_EQ(rmultimap.count(e2), 2);
  }

  // Verify equal_range works
  // Generic
  auto range = map.equal_range(he);
  auto range2 = multimap.equal_range(d2);
  if constexpr (std::same_as<Res, Host>) {
    ASSERT_EQ(std::distance(range.first, range.second), 1);
    ASSERT_EQ(std::distance(range2.first, range2.second), 5);
  } else {
    ASSERT_EQ(std::distance(range.first, range.second), 1);
    ASSERT_EQ(std::distance(range2.first, range2.second), 2);
  }

  // Typed
  auto rrange = rmap.equal_range(e1);
  auto rrange2 = rmultimap.equal_range(e2);
  if constexpr (std::same_as<Res, Host>) {
    ASSERT_EQ(std::distance(rrange.first, rrange.second), 1);
    ASSERT_EQ(std::distance(rrange2.first, rrange2.second), 3);
  } else {
    ASSERT_EQ(std::distance(rrange.first, rrange.second), 1);
    ASSERT_EQ(std::distance(rrange2.first, rrange2.second), 2);
  }
}

//
TEST(CampEvent, UnorderedMapKey)
{
  Event he = Host().get_event_erased();
  test_map_key<Host>(he);
#ifdef CAMP_HAVE_CUDA
  test_map_key<Cuda>(he);
#endif
#ifdef CAMP_HAVE_HIP
  test_map_key<Hip>(he);
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_map_key<Omp>(he);
#endif
#ifdef CAMP_HAVE_SYCL
  test_map_key<Sycl>(he);
#endif
}

template <typename Res>
void test_id_compare(Resource& h1)
{
  Resource r1{Res()};
  Res r;
  Resource r2{r};
  Resource r3{Res(0)};  // should be same as r1

  EXPECT_EQ(r1, r3);

  ASSERT_TRUE(r1 == r1);
  ASSERT_TRUE(r2 == r2);
  ASSERT_TRUE(r1 != r2);
  ASSERT_TRUE(r2 != r1);
  ASSERT_TRUE(r == r);

  ASSERT_FALSE(r1 != r1);
  ASSERT_FALSE(r2 != r2);
  ASSERT_FALSE(r1 == r2);
  ASSERT_FALSE(r2 == r1);
  ASSERT_FALSE(r != r);

  ASSERT_TRUE(r1 != h1);
  ASSERT_TRUE(h1 != r1);

  ASSERT_FALSE(r1 == h1);
  ASSERT_FALSE(h1 == r1);
}

//
TEST(CampResource, Compare)
{
  Resource h1{Host()};
  Host h;
  Resource h2{h};

  ASSERT_TRUE(h1 == h1);
  ASSERT_TRUE(h2 == h2);
  ASSERT_TRUE(h1 == h2);
  ASSERT_TRUE(h2 == h1);
  ASSERT_TRUE(h == h);

  ASSERT_FALSE(h1 != h1);
  ASSERT_FALSE(h2 != h2);
  ASSERT_FALSE(h1 != h2);
  ASSERT_FALSE(h2 != h1);
  ASSERT_FALSE(h != h);

#ifdef CAMP_HAVE_CUDA
  test_id_compare<Cuda>(h1);
#endif
#ifdef CAMP_HAVE_HIP
  test_id_compare<Hip>(h1);
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_id_compare<Omp>(h1);
#endif
#ifdef CAMP_HAVE_SYCL
  test_id_compare<Sycl>(h1);
#endif
}

TEST(CampResource, HostCompare)
{
  Host h1;
  Resource h2{h1};
  Resource h3{Host().get_default()};

  ASSERT_TRUE(Resource{h1} == h2);
  ASSERT_TRUE(Resource{h1} == h3);
  ASSERT_TRUE(h2 == h1);
  ASSERT_TRUE(h2 == h3);
  ASSERT_TRUE(h3 == h1);
  ASSERT_TRUE(h3 == h2);
}


template <typename Res>
void test_id_compare(Event& he)
{
  Event e1 = Res().get_event();
  Event e2 = Res().get_event_erased();
  typename Res::event_type te = Res().get_event();

  ASSERT_TRUE(e1 == e1);
  ASSERT_TRUE(e2 == e2);
  ASSERT_TRUE(e1 != e2);
  ASSERT_TRUE(e2 != e1);
  ASSERT_TRUE(te == te);

  ASSERT_FALSE(e1 != e1);
  ASSERT_FALSE(e2 != e2);
  ASSERT_FALSE(e1 == e2);
  ASSERT_FALSE(e2 == e1);
  ASSERT_FALSE(te != te);

  ASSERT_TRUE(e1 != he);
  ASSERT_TRUE(he != e1);

  ASSERT_FALSE(e1 == he);
  ASSERT_FALSE(he == e1);
}

//
TEST(CampEvent, Compare)
{
  Event e1 = Host().get_event();
  Event e2 = Host().get_event_erased();
  HostEvent te = Host().get_event();

  ASSERT_TRUE(e1 == e1);
  ASSERT_TRUE(e2 == e2);
  ASSERT_TRUE(e1 == e2);
  ASSERT_TRUE(e2 == e1);
  ASSERT_TRUE(te == te);

  ASSERT_FALSE(e1 != e1);
  ASSERT_FALSE(e2 != e2);
  ASSERT_FALSE(e1 != e2);
  ASSERT_FALSE(e2 != e1);
  ASSERT_FALSE(te != te);

#ifdef CAMP_HAVE_CUDA
  test_id_compare<Cuda>(e1);
#endif
#ifdef CAMP_HAVE_HIP
  test_id_compare<Hip>(e1);
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_id_compare<Omp>(e1);
#endif
#ifdef CAMP_HAVE_SYCL
  test_id_compare<Sycl>(e1);
#endif
}

TEST(CampEvent, HostEventCompare)
{
  HostEvent te = Host().get_default().get_event();
  Event e1 = Host().get_event();
  Event e2 = Host().get_event_erased();

  ASSERT_TRUE(e1 == te);
  ASSERT_TRUE(e1 == e2);
  ASSERT_TRUE(e2 == te);
  ASSERT_TRUE(e2 == e1);

  ASSERT_FALSE(e1 != te);
  ASSERT_FALSE(e1 != e2);
  ASSERT_FALSE(e2 != te);
  ASSERT_FALSE(e2 != e1);

  Event e3{std::move(te)};

  ASSERT_TRUE(e3 == e1);
  ASSERT_TRUE(e3 == e2);

  ASSERT_FALSE(e3 != e1);
  ASSERT_FALSE(e3 != e2);
}

template <typename Res>
void test_reassignment()
{
  Resource h1{Host()};
  Resource r1{Res()};
  h1 = Res();
  ASSERT_EQ(typeid(r1), typeid(h1));

  Resource h2{Host()};
  Resource r2{Res()};
  r2 = Host();
  ASSERT_EQ(typeid(r2), typeid(h2));
}

//
TEST(CampResource, Reassignment)
{
  test_reassignment<Host>();
#ifdef CAMP_HAVE_CUDA
  test_reassignment<Cuda>();
#endif
#ifdef CAMP_HAVE_HIP
  test_reassignment<Hip>();
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_reassignment<Omp>();
#endif
#ifdef CAMP_HAVE_SYCL
  test_reassignment<Sycl>();
#endif
}

void test_select_stream(Resource r1, Resource r2)
{
  const int N = 5;
  int* r_array1 = r1.allocate<int>(N);
  int* r_array2 = r2.allocate<int>(N);

  r1.deallocate(r_array1);
  r2.deallocate(r_array2);
}

//
TEST(CampResource, StreamSelect)
{
  test_select_stream(Host(), Host());
#if defined(CAMP_HAVE_CUDA)
  {
    cudaStream_t stream1, stream2;
    CAMP_CUDA_API_INVOKE_AND_CHECK(cudaStreamCreate, &stream1);
    CAMP_CUDA_API_INVOKE_AND_CHECK(cudaStreamCreate, &stream2);
    test_select_stream(Cuda::CudaFromStream(stream1),
                       Cuda::CudaFromStream(stream2));
    CAMP_CUDA_API_INVOKE_AND_CHECK(cudaStreamDestroy, stream1);
    CAMP_CUDA_API_INVOKE_AND_CHECK(cudaStreamDestroy, stream2);
  }
#endif
#if defined(CAMP_HAVE_HIP)
  {
    hipStream_t stream1, stream2;
    CAMP_HIP_API_INVOKE_AND_CHECK(hipStreamCreate, &stream1);
    CAMP_HIP_API_INVOKE_AND_CHECK(hipStreamCreate, &stream2);
    test_select_stream(Hip::HipFromStream(stream1),
                       Hip::HipFromStream(stream2));
    CAMP_HIP_API_INVOKE_AND_CHECK(hipStreamDestroy, stream1);
    CAMP_HIP_API_INVOKE_AND_CHECK(hipStreamDestroy, stream2);
  }
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  {
    char a[2];
    test_select_stream(Omp::OmpFromAddr(&a[0]), Omp::OmpFromAddr(&a[1]));
  }
#endif
#ifdef CAMP_HAVE_SYCL
  {
    auto gpuSelector = sycl::gpu_selector_v;
    sycl::property_list propertyList =
        sycl::property_list(sycl::property::queue::in_order());
    sycl::context context;
    sycl::queue queue1(context, gpuSelector, propertyList);
    sycl::queue queue2(context, gpuSelector, propertyList);
    test_select_stream(Sycl::SyclFromQueue(queue1),
                       Sycl::SyclFromQueue(queue2));
  }
#endif
}

template <typename Res>
void test_get()
{
  Resource dev_res{Res()};
  auto erased_res = dev_res.get<Res>();
  Res pure_res;
  ASSERT_EQ(typeid(erased_res), typeid(pure_res));
}

//
TEST(CampResource, Get)
{
  test_get<Host>();
#ifdef CAMP_HAVE_CUDA
  test_get<Cuda>();
#endif
#ifdef CAMP_HAVE_HIP
  test_get<Hip>();
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_get<Omp>();
#endif
#ifdef CAMP_HAVE_SYCL
  test_get<Sycl>();
#endif
}

template <typename Res>
void test_get_rvalue()
{
  Resource dev_res{Res()};
  auto moved_res = std::move(dev_res).get<Res>();
  ASSERT_EQ(typeid(moved_res), typeid(Res{}));
}

TEST(CampResource, GetRValue)
{
  test_get_rvalue<Host>();
#ifdef CAMP_HAVE_CUDA
  test_get_rvalue<Cuda>();
#endif
#ifdef CAMP_HAVE_HIP
  test_get_rvalue<Hip>();
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_get_rvalue<Omp>();
#endif
#ifdef CAMP_HAVE_SYCL
  test_get_rvalue<Sycl>();
#endif
}

template <typename Res, typename ResEvent, typename... EventArgs>
void test_get_event(EventArgs&&... eventArgs)
{
  Resource r{Res()};
  auto erased_event = r.get_event();
  Event event{ResEvent(std::forward<EventArgs>(eventArgs)...)};
  ASSERT_EQ(typeid(event), typeid(erased_event));
}

//
TEST(CampResource, GetEvent)
{
  test_get_event<Host, HostEvent>();
#if defined(CAMP_HAVE_CUDA)
  {
    cudaStream_t s;
    CAMP_CUDA_API_INVOKE_AND_CHECK(cudaStreamCreate, &s);
    test_get_event<Cuda, CudaEvent>(s);
    CAMP_CUDA_API_INVOKE_AND_CHECK(cudaStreamDestroy, s);
  }
#endif
#if defined(CAMP_HAVE_HIP)
  {
    hipStream_t s;
    CAMP_HIP_API_INVOKE_AND_CHECK(hipStreamCreate, &s);
    test_get_event<Hip, HipEvent>(s);
    CAMP_HIP_API_INVOKE_AND_CHECK(hipStreamDestroy, s);
  }
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  {
    char a[1];
    test_get_event<Omp, OmpEvent>(&a[0]);
  }
#endif
#ifdef CAMP_HAVE_SYCL
  {
    auto gpuSelector = sycl::gpu_selector_v;
    sycl::property_list propertyList =
        sycl::property_list(sycl::property::queue::in_order());
    sycl::context context;
    sycl::queue q(context, gpuSelector, propertyList);
    test_get_event<Sycl, SyclEvent>(q);
  }
#endif
}

template <typename Res, typename ResEvent, typename... EventArgs>
void test_get_typed_event(EventArgs&&... eventArgs)
{
  Resource r{Res()};
  Event erased_event = r.get_event();
  auto&& typed_event = erased_event.get<ResEvent>();
  ResEvent event(std::forward<EventArgs>(eventArgs)...);
  ASSERT_EQ(typeid(event), typeid(typed_event));
}

//
TEST(CampEvent, Get)
{
  test_get_typed_event<Host, HostEvent>();
#if defined(CAMP_HAVE_CUDA)
  {
    cudaStream_t s;
    CAMP_CUDA_API_INVOKE_AND_CHECK(cudaStreamCreate, &s);
    test_get_typed_event<Cuda, CudaEvent>(s);
    CAMP_CUDA_API_INVOKE_AND_CHECK(cudaStreamDestroy, s);
  }
#endif
#if defined(CAMP_HAVE_HIP)
  {
    hipStream_t s;
    CAMP_HIP_API_INVOKE_AND_CHECK(hipStreamCreate, &s);
    test_get_typed_event<Hip, HipEvent>(s);
    CAMP_HIP_API_INVOKE_AND_CHECK(hipStreamDestroy, s);
  }
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  {
    char a[1];
    test_get_typed_event<Omp, OmpEvent>(&a[0]);
  }
#endif
#ifdef CAMP_HAVE_SYCL
  {
    auto gpuSelector = sycl::gpu_selector_v;
    sycl::property_list propertyList =
        sycl::property_list(sycl::property::queue::in_order());
    sycl::context context;
    sycl::queue q(context, gpuSelector, propertyList);
    test_get_typed_event<Sycl, SyclEvent>(q);
  }
#endif
}

template <typename Res>
static EventProxy<Res> do_stuff(Res r)
{
  return EventProxy<Res>(r);
}

//
template <typename Res, typename ResEvent>
void test_event_proxy()
{
  Res r{Res{}};

  {
    EventProxy<Res> ep{r};
    ResEvent e = ep.get();
    CAMP_ALLOW_UNUSED_LOCAL(e);
  }

  {
    EventProxy<Res> ep{r};
    ResEvent e = ep;
    CAMP_ALLOW_UNUSED_LOCAL(e);
  }

  {
    EventProxy<Res> ep{r};
    Event e = ep.get();
    CAMP_ALLOW_UNUSED_LOCAL(e);
  }

  {
    EventProxy<Res> ep{r};
    Event e = ep;
    CAMP_ALLOW_UNUSED_LOCAL(e);
  }

  {
    EventProxy<Resource> ep{Resource{r}};
    Event e = ep.get();
    CAMP_ALLOW_UNUSED_LOCAL(e);
  }

  {
    EventProxy<Resource> ep{Resource{r}};
    Event e = ep;
    CAMP_ALLOW_UNUSED_LOCAL(e);
  }

  {
    ResEvent e = do_stuff(r);
    CAMP_ALLOW_UNUSED_LOCAL(e);
  }

  {
    Event e = do_stuff(r);
    CAMP_ALLOW_UNUSED_LOCAL(e);
  }

  {
    Event e = do_stuff(Resource{r});
    CAMP_ALLOW_UNUSED_LOCAL(e);
  }

  {
    do_stuff(r);
  }
}

//
TEST(CampEventProxy, Get)
{
  test_event_proxy<Host, HostEvent>();
#ifdef CAMP_HAVE_CUDA
  test_event_proxy<Cuda, CudaEvent>();
#endif
#ifdef CAMP_HAVE_HIP
  test_event_proxy<Hip, HipEvent>();
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_event_proxy<Omp, OmpEvent>();
#endif
#ifdef CAMP_HAVE_SYCL
  test_event_proxy<Sycl, SyclEvent>();
#endif
}

template <typename Res>
void test_wait()
{
  auto r = Res();
  r.wait();
  r.wait_for(nullptr);
  Event erased_event = r.get_event_erased();
  r.wait_for(&erased_event);
  auto typed_event = r.get_event();
  r.wait_for(&typed_event);
  Event host_event = Host().get_event_erased();
  r.wait_for(&host_event);
  Resource er(r);
  er.wait();
}

//
TEST(CampResource, Wait)
{
  test_wait<Host>();
#ifdef CAMP_HAVE_CUDA
  test_wait<Cuda>();
#endif
#ifdef CAMP_HAVE_HIP
  test_wait<Hip>();
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_wait<Omp>();
#endif
#ifdef CAMP_HAVE_SYCL
  test_wait<Sycl>();
#endif
}

template <typename Res>
void test_event_wait()
{
  auto r = Res();
  const auto typed_event = r.get_event();
  const Event event = r.get_event_erased();
  typed_event.wait();
  event.wait();
}

//
TEST(CampEvent, Wait)
{
  test_event_wait<Host>();
#ifdef CAMP_HAVE_CUDA
  test_event_wait<Cuda>();
#endif
#ifdef CAMP_HAVE_HIP
  test_event_wait<Hip>();
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_event_wait<Omp>();
#endif
#ifdef CAMP_HAVE_SYCL
  test_event_wait<Sycl>();
#endif
}

template <typename Res>
void test_event_check()
{
  auto r = Res();
  const auto typed_event = r.get_event();
  const Event event = r.get_event_erased();
  // checking in a loop should always eventually return true
  while (!typed_event.check()) {}
  while (!event.check()) {}
}

//
TEST(CampEvent, Check)
{
  test_event_check<Host>();
#ifdef CAMP_HAVE_CUDA
  test_event_check<Cuda>();
#endif
#ifdef CAMP_HAVE_HIP
  test_event_check<Hip>();
#endif
#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_event_check<Omp>();
#endif
#ifdef CAMP_HAVE_SYCL
  test_event_check<Sycl>();
#endif
}

template <typename Res>
void test_concrete_resource_trait()
{
  ASSERT_TRUE(is_concrete_resource<Res>::value);
  ASSERT_TRUE(is_concrete_resource<Res&>::value);
  ASSERT_TRUE(is_concrete_resource<const Res>::value);
  ASSERT_TRUE(is_concrete_resource<const Res&>::value);
  ASSERT_TRUE(is_concrete_resource<Res&&>::value);
}
//
TEST(CampResourceTypeTraits, ConcreteResource)
{
  test_concrete_resource_trait<Host>();

#ifdef CAMP_HAVE_CUDA
  test_concrete_resource_trait<Cuda>();
#endif

#ifdef CAMP_HAVE_HIP
  test_concrete_resource_trait<Hip>();
#endif

#ifdef CAMP_HAVE_SYCL
  test_concrete_resource_trait<Sycl>();
#endif

#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_concrete_resource_trait<Omp>();
#endif

  // Resource is not a concrete resource
  ASSERT_FALSE(is_concrete_resource<Resource>::value);
  // Test is_concrete_resource with non-resource types
  ASSERT_FALSE(is_concrete_resource<int>::value);
  ASSERT_FALSE(is_concrete_resource<float>::value);
  ASSERT_FALSE(is_concrete_resource<double>::value);
  ASSERT_FALSE(is_concrete_resource<void*>::value);
  ASSERT_FALSE(is_concrete_resource<char*>::value);
  ASSERT_FALSE(is_concrete_resource<NotAResource>::value);
  // Host2 has an overload of is_concrete_resource_impl
  ASSERT_TRUE(is_concrete_resource<Host2>::value);
}

template <typename Evt>
void test_concrete_event_trait()
{
  ASSERT_TRUE(is_concrete_event<Evt>::value);
  ASSERT_TRUE(is_concrete_event<Evt&>::value);
  ASSERT_TRUE(is_concrete_event<const Evt>::value);
  ASSERT_TRUE(is_concrete_event<const Evt&>::value);
  ASSERT_TRUE(is_concrete_event<Evt&&>::value);
}
//
TEST(CampResourceTypeTraits, ConcreteEvent)
{
  test_concrete_event_trait<HostEvent>();

#ifdef CAMP_HAVE_CUDA
  test_concrete_event_trait<CudaEvent>();
#endif

#ifdef CAMP_HAVE_HIP
  test_concrete_event_trait<HipEvent>();
#endif

#ifdef CAMP_HAVE_SYCL
  test_concrete_event_trait<SyclEvent>();
#endif

#ifdef CAMP_HAVE_OMP_OFFLOAD
  test_concrete_event_trait<OmpEvent>();
#endif

  // Event is not a concrete event
  ASSERT_FALSE(is_concrete_event<Event>::value);
  // Test is_concrete_event with non-event types
  ASSERT_FALSE(is_concrete_event<int>::value);
  ASSERT_FALSE(is_concrete_event<float>::value);
  ASSERT_FALSE(is_concrete_event<double>::value);
  ASSERT_FALSE(is_concrete_event<void*>::value);
  ASSERT_FALSE(is_concrete_event<char*>::value);
  ASSERT_FALSE(is_concrete_event<NotAnEvent>::value);
  // Host2 has an overload of is_concrete_event_impl
  ASSERT_TRUE(is_concrete_event<HostEvent2>::value);
}

template <typename Res>
void verify_bytes_roundtrip(Res& r, unsigned char* ptr)
{
  constexpr size_t N = 32;
  std::array<unsigned char, N> expected{};
  std::array<unsigned char, N> actual{};

  for (size_t i = 0; i < N; ++i) {
    expected[i] = static_cast<unsigned char>(i * 7 + 3);
  }

  r.memcpy(ptr, expected.data(), N);
  r.wait();
  actual.fill(0);
  r.memcpy(actual.data(), ptr, N);
  r.wait();
  ASSERT_EQ(actual, expected);
}

template <typename Res>
void verify_zero_initialized(Res& r, void* ptr)
{
  constexpr size_t N = 32;
  std::array<unsigned char, N> actual{};
  actual.fill(0xFF);
  r.memcpy(actual.data(), ptr, N);
  r.wait();
  for (auto value : actual) {
    ASSERT_EQ(value, 0u);
  }
}

template <typename Res>
void verify_memset_value(Res& r, void* ptr, unsigned char value)
{
  constexpr size_t N = 32;
  std::array<unsigned char, N> actual{};
  r.memset(ptr, value, N);
  r.wait();
  actual.fill(0);
  r.memcpy(actual.data(), ptr, N);
  r.wait();
  for (auto byte : actual) {
    ASSERT_EQ(byte, value);
  }
}

template <typename Res>
void test_memory_ops(MemoryAccess access)
{
  constexpr size_t N = 32;
  Res r;
  auto* ptr = r.template allocate<unsigned char>(N, access);
  ASSERT_NE(ptr, nullptr);
  verify_memset_value(r, ptr, 0x5A);
  verify_bytes_roundtrip(r, ptr);

  void* zeroed = r.calloc(N, access);
  ASSERT_NE(zeroed, nullptr);
  verify_zero_initialized(r, zeroed);

  r.deallocate(ptr, access);
  r.deallocate(zeroed, access);
}

TEST(CampResourceMemory, Host)
{
  test_memory_ops<Host>(MemoryAccess::Device);
}
#ifdef CAMP_HAVE_CUDA
TEST(CampResourceMemory, Cuda)
{
  test_memory_ops<Cuda>(MemoryAccess::Unknown);
  test_memory_ops<Cuda>(MemoryAccess::Device);
  test_memory_ops<Cuda>(MemoryAccess::Pinned);
  test_memory_ops<Cuda>(MemoryAccess::Managed);
}
#endif

#ifdef CAMP_HAVE_HIP
TEST(CampResourceMemory, Hip)
{
  test_memory_ops<Hip>(MemoryAccess::Unknown);
  test_memory_ops<Hip>(MemoryAccess::Device);
  test_memory_ops<Hip>(MemoryAccess::Pinned);
  test_memory_ops<Hip>(MemoryAccess::Managed);
}
#endif

#ifdef CAMP_HAVE_OMP_OFFLOAD
TEST(CampResourceMemory, Omp)
{
  test_memory_ops<Omp>(MemoryAccess::Device);
}
#endif

#ifdef CAMP_HAVE_SYCL
TEST(CampResourceMemory, Sycl)
{
  test_memory_ops<Sycl>(MemoryAccess::Unknown);
  test_memory_ops<Sycl>(MemoryAccess::Device);
  test_memory_ops<Sycl>(MemoryAccess::Pinned);
  test_memory_ops<Sycl>(MemoryAccess::Managed);
}
#endif
