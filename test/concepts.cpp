//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) Lawrence Livermore National Security, LLC and other
// Camp Project Developers. See top-level LICENSE and COPYRIGHT
// files for dates and other details. No copyright assignment is required
// to contribute to Camp.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include <string>

#include "camp/concepts.hpp"
#include "camp/resource.hpp"

namespace resource_api_test
{
using namespace camp::resources;
using camp::concepts::ConcreteEventAPI;
using camp::concepts::ConcreteResourceAPI;

// Check const/reference normalization with Host; each backend header already
// asserts that its concrete types satisfy the API.
static_assert(camp::concepts::ConcreteResource<Host>);
static_assert(camp::concepts::ConcreteResource<Host&>);
static_assert(camp::concepts::ConcreteResource<const Host>);
static_assert(camp::concepts::ConcreteResource<const Host&>);
static_assert(camp::concepts::ConcreteResource<Host&&>);
static_assert(camp::concepts::ConcreteResource<const Host&&>);
static_assert(camp::concepts::ConcreteEvent<HostEvent>);
static_assert(camp::concepts::ConcreteEvent<HostEvent&>);
static_assert(camp::concepts::ConcreteEvent<const HostEvent>);
static_assert(camp::concepts::ConcreteEvent<const HostEvent&>);
static_assert(camp::concepts::ConcreteEvent<HostEvent&&>);
static_assert(camp::concepts::ConcreteEvent<const HostEvent&&>);

// Start with Host's valid API and change one requirement in each derived
// fixture. ResourceAPIBase supplies get_default() with the fixture's return
// type; inheriting Host::get_default() would otherwise return Host and cause
// an unrelated failure. Event fixtures use HostEvent directly.
//
// A declaration in the derived type hides the inherited member of that name.
// This lets a fixture remove a required call by deleting it or changing its
// arguments, return type, or const qualification, while keeping the rest of
// the API available. Declarations suffice: requires expressions check whether
// calls are valid without executing backend operations or global cleanup.
template <typename Derived>
struct ResourceAPIBase : Host {
  static Derived get_default();
};

// Each type independently violates part of the common API.
struct MissingDefault : ResourceAPIBase<MissingDefault> {
  static void get_default(int);
};

struct MissingCleanup : ResourceAPIBase<MissingCleanup> {
  static void cleanup(int);
};

struct NonstaticDefault : ResourceAPIBase<NonstaticDefault> {
  NonstaticDefault get_default();
};

struct NonstaticCleanup : ResourceAPIBase<NonstaticCleanup> {
  void cleanup();
};

struct WrongEventType : ResourceAPIBase<WrongEventType> {
  using event_type = int;
};

struct ReferenceEventType : ResourceAPIBase<ReferenceEventType> {
  using event_type = HostEvent&;
  HostEvent& get_event();
};

struct WrongNativeEvent : ResourceAPIBase<WrongNativeEvent> {
  int get_event();
};

struct WrongErasedEvent : ResourceAPIBase<WrongErasedEvent> {
  HostEvent get_event_erased();
};

struct MissingWait : ResourceAPIBase<MissingWait> {
  void wait() = delete;
};

// ROCm 6.4.3 HIP mode can select an inherited overload exposed by a using
// declaration even when the exact-match overload here is deleted. Declare
// the surviving overloads directly, including the event-ref overload, so each
// rejection tests the intended call.
struct MissingNativeWait : ResourceAPIBase<MissingNativeWait> {
  void wait_for(HostEvent const&) = delete;
  void wait_for(EventConstRef);
  void wait_for(Event const&);
};

struct MissingErasedWait : ResourceAPIBase<MissingErasedWait> {
  void wait_for(HostEvent const&);
  void wait_for(EventConstRef);
  void wait_for(Event const&) = delete;
};

struct MissingRefWait : ResourceAPIBase<MissingRefWait> {
  void wait_for(HostEvent const&);
  void wait_for(Event const&);
};

struct WrongAllocation : ResourceAPIBase<WrongAllocation> {
  template <typename T>
  void* allocate(std::size_t, MemoryAccess = MemoryAccess::Device);
};

struct MissingDefaultAccess : ResourceAPIBase<MissingDefaultAccess> {
  void* calloc(std::size_t, MemoryAccess);
};

struct WrongCalloc : ResourceAPIBase<WrongCalloc> {
  bool calloc(std::size_t, MemoryAccess = MemoryAccess::Device);
};

struct MissingDeallocate : ResourceAPIBase<MissingDeallocate> {
  void deallocate(void*, MemoryAccess = MemoryAccess::Device) = delete;
};

struct MutableMemcpySource : ResourceAPIBase<MutableMemcpySource> {
  void memcpy(void*, void*, std::size_t);
};

struct MissingMemset : ResourceAPIBase<MissingMemset> {
  void memset(void*, int, std::size_t) = delete;
};

struct MissingCheck : HostEvent {
  bool check() const = delete;
};

struct MutableCheck : HostEvent {
  bool check();
};

struct WrongCheck : HostEvent {
  int check() const;
};

struct MutableWait : HostEvent {
  void wait();
};

struct WrongWait : HostEvent {
  int wait() const;
};

// These inherit get_hash(), but deliberately have no std::hash specialization:
// a backend member alone does not satisfy Hashable.
struct MissingHashResource : ResourceAPIBase<MissingHashResource> {
};

struct MissingHashEvent : HostEvent {
};

// These standalone types exercise Hashable through the specializations below,
// independently of the resource/event API.
struct HashableValue {
};

struct WrongHashResult {
};

struct MutableHashInput {
};

struct DisabledHash {
};

}  // namespace resource_api_test

namespace std
{
template <>
struct hash<resource_api_test::HashableValue> {
  unsigned int operator()(resource_api_test::HashableValue const&) const;
};

template <>
struct hash<resource_api_test::WrongHashResult> {
  void operator()(resource_api_test::WrongHashResult const&) const;
};

template <>
struct hash<resource_api_test::MutableHashInput> {
  size_t operator()(resource_api_test::MutableHashInput&) const;
};

template <>
struct hash<resource_api_test::DisabledHash> {
  hash() = delete;
  size_t operator()(resource_api_test::DisabledHash const&) const;
};

// std::hash specializations are not inherited by derived resource/event types.
// Give the other malformed API fixtures valid hashes by reusing the backend
// hasher, which accepts their base subobject. Otherwise a missing hash could
// make a rejection test pass even if its intended API check were broken.
template <>
struct hash<resource_api_test::MissingDefault> : hash<camp::resources::Host> {
};

template <>
struct hash<resource_api_test::MissingCleanup> : hash<camp::resources::Host> {
};

template <>
struct hash<resource_api_test::NonstaticDefault> : hash<camp::resources::Host> {
};

template <>
struct hash<resource_api_test::NonstaticCleanup> : hash<camp::resources::Host> {
};

template <>
struct hash<resource_api_test::WrongEventType> : hash<camp::resources::Host> {
};

template <>
struct hash<resource_api_test::ReferenceEventType>
    : hash<camp::resources::Host> {
};

template <>
struct hash<resource_api_test::WrongNativeEvent> : hash<camp::resources::Host> {
};

template <>
struct hash<resource_api_test::WrongErasedEvent> : hash<camp::resources::Host> {
};

template <>
struct hash<resource_api_test::MissingWait> : hash<camp::resources::Host> {
};

template <>
struct hash<resource_api_test::MissingNativeWait>
    : hash<camp::resources::Host> {
};

template <>
struct hash<resource_api_test::MissingErasedWait>
    : hash<camp::resources::Host> {
};

template <>
struct hash<resource_api_test::MissingRefWait>
    : hash<camp::resources::Host> {
};

template <>
struct hash<resource_api_test::WrongAllocation> : hash<camp::resources::Host> {
};

template <>
struct hash<resource_api_test::MissingDefaultAccess>
    : hash<camp::resources::Host> {
};

template <>
struct hash<resource_api_test::WrongCalloc> : hash<camp::resources::Host> {
};

template <>
struct hash<resource_api_test::MissingDeallocate>
    : hash<camp::resources::Host> {
};

template <>
struct hash<resource_api_test::MutableMemcpySource>
    : hash<camp::resources::Host> {
};

template <>
struct hash<resource_api_test::MissingMemset> : hash<camp::resources::Host> {
};

template <>
struct hash<resource_api_test::MissingCheck>
    : hash<camp::resources::HostEvent> {
};

template <>
struct hash<resource_api_test::MutableCheck>
    : hash<camp::resources::HostEvent> {
};

template <>
struct hash<resource_api_test::WrongCheck> : hash<camp::resources::HostEvent> {
};

template <>
struct hash<resource_api_test::MutableWait> : hash<camp::resources::HostEvent> {
};

template <>
struct hash<resource_api_test::WrongWait> : hash<camp::resources::HostEvent> {
};
}  // namespace std

namespace resource_api_test
{
// Evaluate the requirements only after the hash specializations are visible.
static_assert(camp::concepts::Hashable<int>);
static_assert(camp::concepts::Hashable<std::string>);
static_assert(camp::concepts::Hashable<HashableValue>);
static_assert(camp::concepts::Hashable<HashableValue const&>);
static_assert(!camp::concepts::Hashable<MissingHashResource>);
static_assert(!camp::concepts::Hashable<MissingHashEvent>);
static_assert(!camp::concepts::Hashable<WrongHashResult>);
static_assert(!camp::concepts::Hashable<MutableHashInput>);
static_assert(!camp::concepts::Hashable<DisabledHash>);
static_assert(!ConcreteResourceAPI<MissingHashResource>);
static_assert(!ConcreteEventAPI<MissingHashEvent>);

static_assert(!ConcreteResourceAPI<MissingDefault>);
static_assert(!ConcreteResourceAPI<MissingCleanup>);
static_assert(!ConcreteResourceAPI<NonstaticDefault>);
static_assert(!ConcreteResourceAPI<NonstaticCleanup>);
static_assert(!ConcreteResourceAPI<WrongEventType>);
static_assert(!ConcreteResourceAPI<ReferenceEventType>);
static_assert(!ConcreteResourceAPI<WrongNativeEvent>);
static_assert(!ConcreteResourceAPI<WrongErasedEvent>);
static_assert(!ConcreteResourceAPI<MissingWait>);
static_assert(!ConcreteResourceAPI<MissingNativeWait>);
static_assert(!ConcreteResourceAPI<MissingErasedWait>);
static_assert(!ConcreteResourceAPI<MissingRefWait>);
static_assert(!ConcreteResourceAPI<WrongAllocation>);
static_assert(!ConcreteResourceAPI<MissingDefaultAccess>);
static_assert(!ConcreteResourceAPI<WrongCalloc>);
static_assert(!ConcreteResourceAPI<MissingDeallocate>);
static_assert(!ConcreteResourceAPI<MutableMemcpySource>);
static_assert(!ConcreteResourceAPI<MissingMemset>);
static_assert(!ConcreteEventAPI<MissingCheck>);
static_assert(!ConcreteEventAPI<MutableCheck>);
static_assert(!ConcreteEventAPI<WrongCheck>);
static_assert(!ConcreteEventAPI<MutableWait>);
static_assert(!ConcreteEventAPI<WrongWait>);
}  // namespace resource_api_test
