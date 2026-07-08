.. ##
.. ## Copyright (c) Lawrence Livermore National Security, LLC and other
.. ## Camp Project Developers. See top-level LICENSE and COPYRIGHT
.. ## files for dates and other details. No copyright assignment is required
.. ## to contribute to Camp.
.. ##
.. ## SPDX-License-Identifier: (BSD-3-Clause)
.. ##

.. _concepts-label:

=======================
Concepts and Type Traits
=======================

Camp provides a small set of C++20 concepts and companion type traits in
``camp/concepts.hpp``. The concepts are useful in ``requires`` clauses and
constrained templates, while the traits provide the same checks in
``std::bool_constant`` form for code that still prefers the traditional
type-trait style.

All concepts live in ``camp::concepts``. All trait wrappers live in
``camp::type_traits``.

------------
Introduction
------------

The API in ``camp/concepts.hpp`` is organized into a few groups:

* comparison concepts such as ``Comparable`` and ``ComparableTo``
* numeric concepts such as ``Integral`` and ``FloatingPoint``
* iterator and range concepts such as ``Iterator`` and ``RandomAccessRange``
* resource concepts such as ``ConcreteResource`` and ``ConcreteEvent``
* trait wrappers such as ``is_iterator_v`` and ``is_comparable_to_v``

In general, use the concept form when constraining templates:

.. code-block:: cpp

   template <camp::concepts::RandomAccessRange R>
   void sort_in_place(R&& range);

Use the trait form when a boolean constant is more convenient:

.. code-block:: cpp

   static_assert(camp::type_traits::is_random_access_range_v<std::vector<int>>);

-------------------
Comparison Concepts
-------------------

Camp provides concepts for common comparison operations:

* ``LessThanComparable``
* ``GreaterThanComparable``
* ``LessEqualComparable``
* ``GreaterEqualComparable``
* ``EqualityComparable``
* ``ComparableTo<T, U>``
* ``Comparable<T>``

``ComparableTo<T, U>`` is the most complete comparison check. It requires the
two types to support all of the standard ordering and equality operators in
both directions, with results convertible to ``bool``.

.. code-block:: cpp

   struct index_t {
     int value;
   };

   inline bool operator<(index_t lhs, index_t rhs) { return lhs.value < rhs.value; }
   inline bool operator<=(index_t lhs, index_t rhs) { return lhs.value <= rhs.value; }
   inline bool operator>(index_t lhs, index_t rhs) { return lhs.value > rhs.value; }
   inline bool operator>=(index_t lhs, index_t rhs) { return lhs.value >= rhs.value; }
   inline bool operator==(index_t lhs, index_t rhs) { return lhs.value == rhs.value; }
   inline bool operator!=(index_t lhs, index_t rhs) { return lhs.value != rhs.value; }

   static_assert(camp::concepts::Comparable<index_t>);
   static_assert(camp::type_traits::is_comparable_v<index_t>);

----------------
Numeric Concepts
----------------

Camp forwards a few common numeric classifications into concept form:

* ``FloatingPoint``
* ``Integral``
* ``Arithmetic``
* ``Signed``
* ``Unsigned``

These are convenient when a template should only accept a narrow category of
scalar types.

.. code-block:: cpp

   template <camp::concepts::Integral T>
   constexpr T midpoint(T a, T b)
   {
     return a + (b - a) / 2;
   }

   static_assert(camp::type_traits::is_integral_v<int>);
   static_assert(!camp::type_traits::is_integral_v<double>);

---------------------------
Iterator and Range Concepts
---------------------------

Camp provides iterator and range concepts that are intended to work with the
project's tuple, array, and resource utilities as well as standard-library
types.

Iterator concepts:

* ``Iterator``
* ``ForwardIterator``
* ``BidirectionalIterator``
* ``RandomAccessIterator``

Range concepts:

* ``HasBeginEnd``
* ``Range``
* ``ForwardRange``
* ``BidirectionalRange``
* ``RandomAccessRange``

These are available both as concepts and type traits:

.. code-block:: cpp

   static_assert(camp::concepts::RandomAccessIterator<int*>);
   static_assert(camp::concepts::RandomAccessRange<std::array<int, 4>>);

   static_assert(camp::type_traits::is_iterator_v<int*>);
   static_assert(camp::type_traits::is_random_access_range_v<std::array<int, 4>>);

``IterableValue<T>`` and ``IteratorValue<T>`` are helper aliases that recover
the dereferenced type from a range-like or iterator-like type.

.. code-block:: cpp

   using range_value = camp::type_traits::IterableValue<std::vector<double>>;
   using iter_value = camp::type_traits::IteratorValue<double*>;

------------------------------------
Concepts and Type Traits for Camp Resources
------------------------------------

Camp also exposes concepts and traits for concrete resource and event types:

* ``camp::concepts::ConcreteResource``
* ``camp::concepts::ConcreteEvent``
* ``camp::resources::is_concrete_resource<T>``
* ``camp::resources::is_concrete_resource_v<T>``
* ``camp::resources::is_concrete_event<T>``
* ``camp::resources::is_concrete_event_v<T>``

These are primarily used by Camp's resource API to distinguish typed backend
objects from the generic type-erased wrappers.

.. code-block:: cpp

   template <camp::concepts::ConcreteResource Res>
   void submit_work(Res&& res);

---------------------------------
Template Specialization Utilities
---------------------------------

``camp::type_traits::IsSpecialized`` and ``camp::type_traits::SpecializationOf``
help with template-pattern matching at compile time.

.. code-block:: cpp

   static_assert(camp::type_traits::IsSpecialized<std::tuple, int, double>::value);
   static_assert(camp::type_traits::SpecializationOf<std::tuple,
                                                     std::tuple<int, double>>::value);

These utilities are useful when a template should accept "any specialization of
template X" rather than a single concrete instantiation.
