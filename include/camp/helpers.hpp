//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2018-25, Lawrence Livermore National Security, LLC
// and Camp project contributors. See the camp/LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef CAMP_HELPERS_HPP
#define CAMP_HELPERS_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "camp/config.hpp"
#include "camp/defines.hpp"

namespace camp
{

/// metafunction to get instance of pointer type
template <typename T>
T* declptr();

/// metafunction to get instance of value type
template <typename T>
CAMP_HOST_DEVICE auto val() noexcept -> decltype(std::declval<T>());

/// metafunction to get instance of const type
template <typename T>
CAMP_HOST_DEVICE auto cval() noexcept -> decltype(std::declval<T const>());

/// metafunction to expand a parameter pack and ignore result
template <typename... Ts>
CAMP_HOST_DEVICE constexpr inline void sink(Ts const&...)
{
}

namespace detail
{
  using __expand_array_type = int[];
}

#define CAMP_EXPAND(...) \
  static_cast<void>(     \
      ::camp::detail::__expand_array_type{0, ((void)(__VA_ARGS__), 0)...})

template <typename Fn, typename... Args>
CAMP_HOST_DEVICE constexpr void for_each_arg(Fn&& f, Args&&... args)
{
  CAMP_EXPAND(f((Args&&)args));
}

// bring common utility routines into scope to allow ADL
using std::begin;
using std::swap;

namespace type
{
  namespace ref
  {
    template <class T>
    struct rem_s {
      using type = T;
    };

    template <class T>
    struct rem_s<T&> {
      using type = T;
    };

    template <class T>
    struct rem_s<T&&> {
      using type = T;
    };

    /// remove reference from T
    template <class T>
    using rem = typename rem_s<T>::type;

    /// add remove reference to T
    template <class T>
    using add = T&;
  }  // end namespace ref

  namespace rvref
  {
    /// add rvalue reference to T
    template <class T>
    using add = T&&;
  }  // end namespace rvref

  namespace ptr
  {
    template <class T>
    struct rem_s {
      using type = T;
    };

    template <class T>
    struct rem_s<T*> {
      using type = T;
    };

    /// remove pointer from T
    template <class T>
    using rem = typename rem_s<T>::type;

    /// add remove pointer to T
    template <class T>
    using add = T*;
  }  // end namespace ptr

  namespace c
  {
    template <class T>
    struct rem_s {
      using type = T;
    };

    template <class T>
    struct rem_s<const T> {
      using type = T;
    };

    /// remove const qualifier from T
    template <class T>
    using rem = typename rem_s<T>::type;

    /// add const qualifier to T
    template <class T>
    using add = const T;
  }  // namespace c

  namespace v
  {
    template <class T>
    struct rem_s {
      using type = T;
    };

    template <class T>
    struct rem_s<volatile T> {
      using type = T;
    };

    /// remove volatile qualifier from T
    template <class T>
    using rem = typename rem_s<T>::type;

    /// add volatile qualifier to T
    template <class T>
    using add = volatile T;
  }  // namespace v

  namespace cv
  {
    template <class T>
    struct rem_s {
      using type = T;
    };

    template <class T>
    struct rem_s<const T> {
      using type = T;
    };

    template <class T>
    struct rem_s<volatile T> {
      using type = T;
    };

    template <class T>
    struct rem_s<const volatile T> {
      using type = T;
    };

    /// remove const and volatile qualifiers from T
    template <class T>
    using rem = typename rem_s<T>::type;

    /// add const and volatile qualifiers to T
    template <class T>
    using add = const volatile T;
  }  // namespace cv
}  // end namespace type

template <typename T>
using decay = type::cv::rem<type::ref::rem<T>>;

template <typename T>
using plain = type::ref::rem<T>;

template <typename T>
using diff_from = decltype(val<plain<T>>() - val<plain<T>>());
template <typename T, typename U>
using diff_between = decltype(val<plain<T>>() - val<plain<U>>());

template <typename T>
using iterator_from = decltype(begin(val<plain<T>>()));

template <class T>
CAMP_HOST_DEVICE constexpr T&& forward(type::ref::rem<T>& t) noexcept
{
  return static_cast<T&&>(t);
}

template <class T>
CAMP_HOST_DEVICE constexpr T&& forward(type::ref::rem<T>&& t) noexcept
{
  return static_cast<T&&>(t);
}

template <typename T>
CAMP_HOST_DEVICE constexpr type::ref::rem<T>&& move(T&& t) noexcept
{
  return static_cast<type::ref::rem<T>&&>(t);
}

template <typename T>
CAMP_HOST_DEVICE
    typename std::enable_if<std::is_move_constructible<T>::value
                            && std::is_move_assignable<T>::value>::type
    safe_swap(T& t1,
              T& t2) noexcept(std::is_nothrow_move_constructible<T>::value
                              && std::is_nothrow_move_assignable<T>::value)
{
#if defined(__CUDA_ARCH__) || defined(__HIP_DEVICE_COMPILE__)
  T temp{std::move(t1)};
  t1 = std::move(t2);
  t2 = std::move(temp);
#else
  using std::swap;
  swap(t1, t2);
#endif
}

template <typename T, typename = decltype(sink(swap(val<T>(), val<T>())))>
CAMP_HOST_DEVICE void safe_swap(T& t1, T& t2)
{
  using std::swap;
  swap(t1, t2);
}

namespace experimental
{

  //! Printing helper class to add printability to types defined outside of
  //! camp.
  //
  //  Specialize to customize printing or add printing for a type. This avoids
  //  conflicts if an operator<< is later added for the type in question.
  //  Write specializations for non-const reference and const-reference.
  template <typename T>
  struct StreamInsertHelper {
    static_assert(std::is_lvalue_reference_v<T>);

    T m_val;

    std::ostream& operator()(std::ostream& str) const { return str << m_val; }
  };

  // deduction guide for StreamInsertHelper
  template <typename T>
  StreamInsertHelper(T& val) -> StreamInsertHelper<T&>;
  template <typename T>
  StreamInsertHelper(T const& val) -> StreamInsertHelper<T const&>;
  template <typename T>
  StreamInsertHelper(T&& val) -> StreamInsertHelper<T const&>;
  template <typename T>
  StreamInsertHelper(T const&& val) -> StreamInsertHelper<T const&>;

  // Allow printing of StreamInsertHelper using its call operator
  template <typename T>
  inline std::ostream& operator<<(std::ostream& str,
                                  StreamInsertHelper<T> const& si)
  {
    return si(str);
  }



#ifdef CAMP_ENABLE_CUDA
  
// SGS Question should the specialization for CUDA 13 cudaMemLocation
// I/O be in Camp?  And should the method be defined here?
//  
// Invoking CUDA methods via CAMP_CUDA_API_INVOKE_AND_CHECK() may have
// cudaMemLocation arguments which require I/O for error reporting
// messages.  In CUDA < 13 these were simple enum values but in CUDA
// 13 cudaMemLocation is a struct.
//  
// SGS Question Is camp requiring C++20 as well so use "requires std::is_same_v()" simplify non-const / const specialization?
//  
// Could simplify the two methods to be :  
//  template <typename T>
//  struct StreamInsertHelper<T&>
//  requires std::is_same_v<std::remove_cv_t<T>, cudaMemLocation>
//  {
//    T& m_val;
//
//    std::ostream& operator()(std::ostream& str) const {
//      return print_cudaMemLocation(str, m_val);
//    }
//  };

#if CUDART_VERSION >= 13000

  //! Convert CUDA cudaMemLocationType to string
  inline const char* to_string(cudaMemLocationType t)
  {
    switch (t) {
      case cudaMemLocationTypeInvalid:
	return "Invalid";
      case cudaMemLocationTypeDevice:
	return "Device";
      case cudaMemLocationTypeHost:
	return "Host";
      case cudaMemLocationTypeHostNuma:
	return "HostNuma";
      default:
	return "Unknown";
    }
  }

  //! Print helper for cudaMemLocation
  inline std::ostream& print_cudaMemLocation(std::ostream& os,
					     const cudaMemLocation& loc)
  {
    os << "cudaMemLocation{type=" << to_string(loc.type)
       << ", id=" << loc.id << "}";
    return os;
  }

  //! Specialization for printing of cudaMemLocation&
  //
  // Invoking CUDA methods via CAMP_CUDA_API_INVOKE_AND_CHECK() may
  // have cudaMemLocation arguments which require I/O for error
  // reporting messages.  CUDA does not supply an operator<< for
  // cudaMemLocation.  Specialize the StreamInsertHelper rather than
  // operator<< to avoid conflicts.
  template <>
  struct StreamInsertHelper<cudaMemLocation&> {
    cudaMemLocation& m_val;

    std::ostream& operator()(std::ostream& str) const {
      return print_cudaMemLocation(str, m_val);
    }
  };

  //! Specialization for printing of const cudaMemLocation&
  //
  // Invoking CUDA methods via CAMP_CUDA_API_INVOKE_AND_CHECK() may
  // have cudaMemLocation arguments which require I/O for error
  // reporting messages.  CUDA does not supply an operator<< for
  // cudaMemLocation.  Specialize the StreamInsertHelper rather than
  // operator<< to avoid conflicts.
  template <>
  struct StreamInsertHelper<const cudaMemLocation&> {
    const cudaMemLocation& m_val;

    std::ostream& operator()(std::ostream& str) const {
      return print_cudaMemLocation(str, m_val);
    }
  };
#endif // CUDART_VERSION >= 13000

  //! Get the argument names for the given cuda API function name.
  //
  //  Returns a space separated string of the arguments to the given function.
  //  Returns an empty string if func is unknown.
  constexpr std::string_view cuda_get_api_arg_names(std::string_view func)
  {
    using storage_type = std::pair<const char*, const char*>;
    constexpr storage_type known_functions[]{
        {"cudaDeviceSynchronize", ""},
        {"cudaGetDevice", "device"},
        {"cudaGetDeviceProperties", "prop device"},
        {"cudaSetDevice", "device"},
        {"cudaStreamCreate", "pStream"},
        {"cudaStreamSynchronize", "stream"},
        {"cudaStreamWaitEvent", "stream event flags"},
        {"cudaEventCreateWithFlags", "event flags"},
        {"cudaEventSynchronize", "event"},
        {"cudaEventQuery", "event"},
        {"cudaEventRecord", "event stream"},
        {"cudaHostAlloc", "pHost size flags"},
        {"cudaMallocHost", "ptr size"},
        {"cudaMalloc", "devPtr size"},
        {"cudaMallocManaged", "devPtr size flags"},
        {"cudaHostFree", "ptr"},
        {"cudaFree", "devPtr"},
        {"cudaMemset", "devPtr value count"},
        {"cudaMemcpy", "dst src count kind"},
        {"cudaMemsetAsync", "devPtr value count stream"},
        {"cudaMemcpyAsync", "dst src count kind stream"},
        {"cudaLaunchKernel", "func gridDim blockDim args sharedMem stream"},
        {"cudaPeekAtLastError", ""},
        {"cudaGetLastError", ""},
        {"cudaFuncGetAttributes", "attr func"},
        {"cudaOccupancyMaxPotentialBlockSize",
         "minGridSize blockSize func dynamicSMemSize blockSizeLimit"},
        {"cudaOccupancyMaxActiveBlocksPerMultiprocessor",
         "numBlocks func blockSize dynamicSMemSize"}};
    for (auto [api_name, api_args] : known_functions) {
      if (func == api_name) {
        return api_args;
      }
    }
    return "";
  }

#endif  // #ifdef CAMP_ENABLE_CUDA

#ifdef CAMP_ENABLE_HIP

  //! Get the argument names for the given hip API function name.
  //
  //  Returns a space separated string of the arguments to the given function.
  //  Returns an empty string if func is unknown.
  constexpr std::string_view hip_get_api_arg_names(std::string_view func)
  {
    using storage_type = std::pair<const char*, const char*>;
    constexpr storage_type known_functions[]{
        {"hipDeviceSynchronize", ""},
        {"hipGetDevice", "device"},
        {"hipGetDeviceProperties", "prop device"},
        {"hipGetDevicePropertiesR0600", "prop device"},
        {"hipSetDevice", "device"},
        {"hipStreamCreate", "pStream"},
        {"hipStreamSynchronize", "stream"},
        {"hipStreamWaitEvent", "stream event flags"},
        {"hipEventCreateWithFlags", "event flags"},
        {"hipEventSynchronize", "event"},
        {"hipEventQuery", "event"},
        {"hipEventRecord", "event stream"},
        {"hipHostMalloc", "pHost size flags"},
        {"hipMalloc", "devPtr size"},
        {"hipMallocManaged", "devPtr size flags"},
        {"hipHostFree", "ptr"},
        {"hipFree", "devPtr"},
        {"hipMemset", "devPtr value count"},
        {"hipMemcpy", "dst src count kind"},
        {"hipMemsetAsync", "devPtr value count stream"},
        {"hipMemcpyAsync", "dst src count kind stream"},
        {"hipLaunchKernel", "func gridDim blockDim args sharedMem stream"},
        {"hipPeekAtLastError", ""},
        {"hipGetLastError", ""},
        {"hipFuncGetAttributes", "attr func"},
        {"hipOccupancyMaxPotentialBlockSize",
         "minGridSize blockSize func dynamicSMemSize blockSizeLimit"},
        {"hipOccupancyMaxActiveBlocksPerMultiprocessor",
         "numBlocks func blockSize dynamicSMemSize"}};
    for (auto [api_name, api_args] : known_functions) {
      if (func == api_name) {
        return api_args;
      }
    }
    return "";
  }

#endif  // #ifdef CAMP_ENABLE_HIP


  //! Generate a string for the given args
  //
  //  This function generates a string for the given argument names and
  //  arguments. Uses StreamInsertHelper to stringify the types in args.
  template <typename Tuple, std::size_t... Is>
  std::ostream& insertArgsString(std::ostream& str,
                                 std::string_view arg_names,
                                 Tuple&& args_tuple,
                                 std::index_sequence<Is...>)
  {
    auto insert_arg = [&, first = true](auto&& arg) mutable {
      if (first) {
        first = false;
      } else {
        str << ", ";
      }
      if (!arg_names.empty()) {
        auto arg_name_size = arg_names.find(' ');
        if (arg_name_size > arg_names.size()) {
          arg_name_size = arg_names.size();
        }
        str << arg_names.substr(0, arg_name_size) << "=";
        if (arg_name_size < arg_names.size()) {
          ++arg_name_size;  // skip space
        }
        arg_names.remove_prefix(arg_name_size);
      }
      str << ::camp::experimental::StreamInsertHelper{arg};
    };
    ::camp::sink(insert_arg);  // suppress unused warning from nvcc
    using std::get;
    (..., insert_arg(get<Is>(std::forward<Tuple>(args_tuple))));
    return str;
  }

}  // namespace experimental

/// Report hip errors by throwing an exception or printing to cerr
///
/// This function generates an error message by getting a string for the given
/// hip error code, function, argument names, arguments, and source location
/// information. Uses StreamInsertHelper to stringify the types in args.
///
/// This function throws an exception if abort is true otherwise prints to cerr.
template <typename Tuple>
void reportError(std::string_view error_name,
                 std::string_view error_description,
                 std::string_view func_name,
                 std::string_view arg_names,
                 Tuple&& args_tuple,
                 std::string_view file,
                 int line,
                 bool abort = true)
{
  std::ostringstream str;
  str << error_name << " error: " << error_description;
  str << " " << func_name << "(";
  ::camp::experimental::insertArgsString(
      str,
      arg_names,
      std::forward<Tuple>(args_tuple),
      std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>{});
  str << ") " << file << ":" << line;
  if (abort) {
    ::camp::throw_re(str.str());
  } else {
    std::cerr << str.str();
  }
}

}  // namespace camp

#endif /* CAMP_HELPERS_HPP */
