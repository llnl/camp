//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include "camp/resource.hpp"

#include <type_traits>

#include "camp/camp.hpp"
#include "gtest/gtest.h"

using namespace camp::resources;

// compatible but different resource for conversion test
struct Host2 : Host {
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
}  // namespace resources
}  // namespace camp

struct NotAResource { };

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
#if defined(CAMP_HAVE_CUDA)
  test_map_key<Cuda>(h);
#elif defined(CAMP_HAVE_HIP)
  test_map_key<Hip>(h);
#elif defined(CAMP_HAVE_OMP_OFFLOAD)
  test_map_key<Omp>(h);
#elif defined(CAMP_HAVE_SYCL)
  test_map_key<Sycl>(h);
#endif
}

template <typename Res>
void test_map_key(Event& he)
{
  // Generic
  std::unordered_map<Event, size_t> map;
  std::unordered_multimap<Event, size_t> multimap;
  Event d1{Res().get_event_erased()};
  Event d2{Res().get_event_erased()};

  // Typed
  auto e1{Res().get_event()};
  auto e2{Res().get_event()};
  std::unordered_map<decltype(e1), size_t> rmap;
  std::unordered_multimap<decltype(e2), size_t> rmultimap;

  // Generic
  map.insert({he, 10});
  multimap.insert({he, 10});
  map.insert({he, 20});
  multimap.insert({he, 20});
  map.insert({d1, 30});
  multimap.insert({d1, 30});
  map.insert({d2, 40});
  multimap.insert({d2, 40});
  map.insert({d2, 50});
  multimap.insert({d2, 50});

  // Typed
  rmap.insert({e1, 30});
  rmultimap.insert({e1, 30});
  rmap.insert({e2, 40});
  rmultimap.insert({e2, 40});
  rmap.insert({e2, 50});
  rmultimap.insert({e2, 50});

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
  Event he{Host().get_event_erased()};
  test_map_key<Host>(he);
#if defined(CAMP_HAVE_CUDA)
  test_map_key<Cuda>(he);
#elif defined(CAMP_HAVE_HIP)
  test_map_key<Hip>(he);
#elif defined(CAMP_HAVE_OMP_OFFLOAD)
  test_map_key<Omp>(he);
#elif defined(CAMP_HAVE_SYCL)
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
  Event e1{Res().get_event()};
  auto te = Res().get_event();
  Event e2{te};

  ASSERT_TRUE(e1 == e1);
  ASSERT_TRUE(e2 == e2);
  ASSERT_TRUE(e1 != e2);
  ASSERT_TRUE(e2 != e1);
  ASSERT_TRUE(te == te);

  ASSERT_FALSE(e1 != e1);
  ASSERT_FALSE(e2 != e2);
  ASSERT_FALSE(e1 == e2);
  ASSERT_FALSE(e2 == e1);
  ASSERT_FALSE(te!= te);

  ASSERT_TRUE(e1 != he);
  ASSERT_TRUE(he != e1);

  ASSERT_FALSE(e1 == he);
  ASSERT_FALSE(he == e1);
}

//
TEST(CampEvent, Compare)
{
  Event e1{Host().get_event_erased()};
  auto te = Host().get_event();
  Event e2{te};

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
  Event e2{Host().get_event()};
  Event e3{Host().get_event_erased()};

  ASSERT_TRUE(Event{te} == e2);
  ASSERT_TRUE(Event{te} == e3);
  ASSERT_TRUE(e2 == te);
  ASSERT_TRUE(e2 == e3);
  ASSERT_TRUE(e3 == te);
  ASSERT_TRUE(e3 == e2);
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
  auto typed_event = erased_event.get<ResEvent>();
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
  Event event = r.get_event_erased();
  r.wait_for(&event);
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
