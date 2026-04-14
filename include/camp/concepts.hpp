//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2018-25, Lawrence Livermore National Security, LLC
// and Camp project contributors. See the camp/LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef CAMP_CONCEPTS_HPP
#define CAMP_CONCEPTS_HPP

#include <iterator>
#include <type_traits>

#include "camp/helpers.hpp"
#include "camp/list.hpp"
#include "camp/number.hpp"
#include "camp/type_traits/is_same.hpp"

namespace camp
{

namespace concepts
{

  namespace metalib
  {
    using camp::is_same;

    /// negation metafunction of a value type
    template <typename T>
    struct negate_t : num<!T::value> {
    };

    /// all_of metafunction of a value type list -- all must be "true"
    template <bool... Bs>
    struct all_of : metalib::is_same<list<t, num<Bs>...>, list<num<Bs>..., t>> {
    };

    /// none_of metafunction of a value type list -- all must be "false"
    template <bool... Bs>
    struct none_of
        : metalib::is_same<idx_seq<false, Bs...>, idx_seq<Bs..., false>> {
    };

    /// any_of metafunction of a value type list -- at least one must be "true""
    template <bool... Bs>
    struct any_of : negate_t<none_of<Bs...>> {
    };

    /// all_of metafunction of a bool list -- all must be "true"
    template <typename... Bs>
    struct all_of_t : all_of<Bs::value...> {
    };

    /// none_of metafunction of a bool list -- all must be "false"
    template <typename... Bs>
    struct none_of_t : none_of<Bs::value...> {
    };

    /// any_of metafunction of a bool list -- at least one must be "true""
    template <typename... Bs>
    struct any_of_t : any_of<Bs::value...> {
    };

  }  // end namespace metalib

}  // end namespace concepts
}  // end namespace camp

#define DefineConcept(CONCEPT_NAME, expr) \
template<typename T> \
concept CONCEPT_NAME = requires(T arg) { expr };

#define DefineConceptBool(CONCEPT_NAME, expr) \
template<typename T> \
concept CONCEPT_NAME = static_cast<bool>(expr);

#define DefineConceptBinary(CONCEPT_NAME, expr1, expr2) \
template<typename T> \
concept CONCEPT_NAME = expr1 && expr2;

#define DefineConceptVar(CONCEPT_NAME, ...) \
template<typename T> \
concept CONCEPT_NAME = EXPAND_AND(__VA_ARGS__);

#define DefineTypeTraitFromConcept(TTName, ConceptName)             \
  template <typename... Args>                                       \
  struct TTName : camp::concepts::requires_<ConceptName, Args...> { \
  }

namespace camp
{
namespace concepts
{

  namespace detail
  {

    template <class...>
    struct TL {
    };

    template <class...>
    struct voider {
      using type = void;
    };

    template <class Default,
              class /* always void*/,
              template <class...> class Concept,
              class TArgs>
    struct detector {
      using value_t = false_type;
      using type = Default;
    };

    template <class Default, template <class...> class Concept, class... Args>
    struct detector<Default,
                    typename voider<Concept<Args...>>::type,
                    Concept,
                    TL<Args...>> {
      using value_t = true_type;
      using type = Concept<Args...>;
    };

    template <template <class...> class Concept, class TArgs>
    using is_detected = detector<void, void, Concept, TArgs>;

    template <template <class...> class Concept, class TArgs>
    using detected = typename is_detected<Concept, TArgs>::value_t;


    template <typename Ret, typename T>
    Ret returns(T const &) noexcept;

  }  // end namespace detail

  template <typename T>
  using negate = metalib::negate_t<T>;

  /// metafunction for use within decltype expression to validate return type is
  /// convertible to given type
  template <typename T, typename U>
  constexpr auto convertible_to(U &&u) noexcept
      -> decltype(detail::returns<camp::true_type>(static_cast<T>((U &&)u)));

  /// metafunction for use within decltype expression to validate type of
  /// expression
  template <typename T, typename U>
  constexpr auto has_type(U &&) noexcept -> metalib::is_same<T, U>;

  template <typename BoolLike>
  constexpr auto is(BoolLike) noexcept
      -> camp::if_<BoolLike, camp::true_type, camp::false_type>;

  template <typename BoolLike>
  constexpr auto is_not(BoolLike) noexcept
      -> camp::if_c<!BoolLike::value, camp::true_type, camp::false_type>;

  /// metaprogramming concept for SFINAE checking of aggregating concepts
  template <typename... Args>
  struct all_of : metalib::all_of_t<Args...> {
  };

  /// metaprogramming concept for SFINAE checking of aggregating concepts
  template <typename... Args>
  struct none_of : metalib::none_of_t<Args...> {
  };

  /// metaprogramming concept for SFINAE checking of aggregating concepts
  template <typename... Args>
  struct any_of : metalib::any_of_t<Args...> {
  };

  /// SFINAE multiple type traits
  template <typename... Args>
  using enable_if = typename std::enable_if<all_of<Args...>::value, void>::type;

  template <typename T, typename... Args>
  using enable_if_t = typename std::enable_if<all_of<Args...>::value, T>::type;

  /// SFINAE concept checking
  template <template <class...> class Op, class... Args>
  struct requires_ : detail::detected<Op, detail::TL<Args...>> {
  };

  DefineConcept(Swappable, swap(arg, arg);)
  DefineConcept(LessThanComparable, {arg < arg} -> std::convertible_to<bool>; )
  DefineConcept(GreaterThanComparable, {arg > arg} -> std::convertible_to<bool>; )
  DefineConcept(LessEqualComparable,  {arg <= arg} -> std::convertible_to<bool>; )
  DefineConcept(GreaterEqualComparable,  {arg >= arg} -> std::convertible_to<bool>; )
  DefineConcept(EqualityComparable,  {arg == arg} -> std::convertible_to<bool>; )

  template <typename T, typename U>
  concept ComparableTo = requires (T lhs, U rhs) {
        {lhs < val<T>()} -> std::convertible_to<bool>;
        {lhs < val<U>()} -> std::convertible_to<bool>;
        {lhs <= val<T>()} -> std::convertible_to<bool>;
        {lhs <= val<U>()} -> std::convertible_to<bool>;
        {lhs > val<T>()} -> std::convertible_to<bool>;
        {lhs > val<U>()} -> std::convertible_to<bool>;
        {lhs >= val<T>()} -> std::convertible_to<bool>;
        {lhs >= val<U>()} -> std::convertible_to<bool>;
        {lhs == val<T>()} -> std::convertible_to<bool>;
        {lhs == val<U>()} -> std::convertible_to<bool>;
        {lhs != val<T>()} -> std::convertible_to<bool>;
        {lhs != val<U>()} -> std::convertible_to<bool>;
  };

  template <typename T>
  concept Comparable = ComparableTo<T, T>;

  DefineConceptBool(Arithmetic, std::is_arithmetic<T>())
  DefineConceptBool(FloatingPoint, std::is_floating_point<T>())
  DefineConceptBool(Integral, std::is_integral<T>())
  DefineConceptBool(Signed, std::is_signed<T>())
  DefineConceptBool(Unsigned, std::is_unsigned<T>())

  DefineConceptBinary(Iterator, !Integral<T>, requires(T arg, T& arg_ref) {
    *arg;
    { ++arg_ref } -> std::same_as<T&>;
  })
  DefineConceptBinary(ForwardIterator, Iterator<T>, requires(T& arg) {
    arg++;
    *arg++;
  })

  DefineConceptBinary(BidirectionalIterator, ForwardIterator<T>, requires(T& arg) {
    { --arg } -> std::same_as<T&>;
    { arg-- } -> std::convertible_to<T const &>;
    *arg--;
  })

  template<typename T>
  concept RandomAccessIterator =
    BidirectionalIterator<T> &&
    Comparable<T> &&
    requires(T arg, T& arg_ref, diff_from<T> increment) {
        // Reference increment requirements
        {arg_ref += increment} -> std::same_as<T&>;
        {arg_ref -= increment} ->std::same_as<T&>;
        // Value increment requirements
        {arg + increment} -> std::same_as<T>;
        {increment + arg} -> std::same_as<T>;
        {arg - increment} -> std::same_as<T>;
        // random access index operator
        arg[increment];
    };

  template <typename T>
  concept HasBeginEnd = requires(T arg) {
    std::begin(arg);
    std::end(arg);
  };


  DefineConceptBinary(Range,  HasBeginEnd<T>, Iterator<iterator_from<T>>)
  DefineConceptBinary(ForwardRange,  HasBeginEnd<T>,
    ForwardIterator<iterator_from<T>>)
  DefineConceptBinary(BidirectionalRange,  HasBeginEnd<T>,
    BidirectionalIterator<iterator_from<T>>)
  DefineConceptBinary(RandomAccessRange, HasBeginEnd<T>, RandomAccessIterator<iterator_from<T>>)

}  // end namespace concepts

namespace type_traits
{
  DefineTypeTraitFromConcept(is_iterator, camp::concepts::Iterator);
  DefineTypeTraitFromConcept(is_forward_iterator,
                             camp::concepts::ForwardIterator);
  DefineTypeTraitFromConcept(is_bidirectional_iterator,
                             camp::concepts::BidirectionalIterator);
  DefineTypeTraitFromConcept(is_random_access_iterator,
                             camp::concepts::RandomAccessIterator);

  DefineTypeTraitFromConcept(is_range, camp::concepts::Range);
  DefineTypeTraitFromConcept(is_forward_range, camp::concepts::ForwardRange);
  DefineTypeTraitFromConcept(is_bidirectional_range,
                             camp::concepts::BidirectionalRange);
  DefineTypeTraitFromConcept(is_random_access_range,
                             camp::concepts::RandomAccessRange);

  DefineTypeTraitFromConcept(is_comparable, camp::concepts::Comparable);
  DefineTypeTraitFromConcept(is_comparable_to, camp::concepts::ComparableTo);

  DefineTypeTraitFromConcept(is_arithmetic, camp::concepts::Arithmetic);
  DefineTypeTraitFromConcept(is_floating_point, camp::concepts::FloatingPoint);
  DefineTypeTraitFromConcept(is_integral, camp::concepts::Integral);
  DefineTypeTraitFromConcept(is_signed, camp::concepts::Signed);
  DefineTypeTraitFromConcept(is_unsigned, camp::concepts::Unsigned);

  template <typename T>
  using IterableValue = decltype(*std::begin(camp::val<T>()));

  template <typename T>
  using IteratorValue = decltype(*camp::val<T>());

  namespace detail
  {

    /// \cond
    template <typename, template <typename...> class, typename...>
    struct IsSpecialized : camp::false_type {
    };

    template <template <typename...> class Template, typename... T>
    struct IsSpecialized<typename concepts::detail::voider<
                             decltype(camp::val<Template<T...>>())>::type,
                         Template,
                         T...> : camp::true_type {
    };

    template <template <class...> class,
              template <class...> class,
              bool,
              class...>
    struct SpecializationOf : camp::false_type {
    };

    template <template <class...> class Expected,
              template <class...> class Actual,
              class... Args>
    struct SpecializationOf<Expected, Actual, true, Args...>
        : camp::concepts::metalib::is_same<Expected<Args...>, Actual<Args...>> {
    };

    /// \endcond

  }  // end namespace detail


  template <template <class...> class Outer, class... Args>
  using IsSpecialized = detail::IsSpecialized<void, Outer, Args...>;

  template <template <class...> class, typename T>
  struct SpecializationOf : camp::false_type {
  };

  template <template <class...> class Expected,
            template <class...> class Actual,
            class... Args>
  struct SpecializationOf<Expected, Actual<Args...>>
      : detail::SpecializationOf<Expected,
                                 Actual,
                                 IsSpecialized<Expected, Args...>::value,
                                 Args...> {
  };

}  // end namespace type_traits
}  // namespace camp

#endif /* CAMP_CONCEPTS_HPP */
