.. ##
.. ## Copyright (c) Lawrence Livermore National Security, LLC and other
.. ## Camp Project Developers. See top-level LICENSE and COPYRIGHT
.. ## files for dates and other details. No copyright assignment is required
.. ## to contribute to Camp.
.. ##
.. ## SPDX-License-Identifier: (BSD-3-Clause)
.. ##

.. _resources-label:

=========
Resources
=========

Camp Resources allow users to keep track of `streams of execution`. A single `stream of execution` on the device 
(e.g. a single CUDA stream) corresponds to a single Camp device resource. Similarly, when we are executing on the 
host, this corresponds to a separate `stream of execution` and therefore a separate Camp host resource.

Typically, we deal with multiple Camp resources. This includes a single resource for the host and one or more for 
the device, depending on how many (CUDA, Hip, etc.) streams we have in use. While we can have multiple Camp resources 
for the device (e.g. multiple CUDA streams), we can only have one resource for the host because the host only has one `stream of execution`.

In addition to the Host, Camp provides a resource for several device backends:

* CUDA
* HIP
* OpenMP Target
* SYCL

Generic vs. Concrete Camp Resources
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Camp has concrete (typed) resources and type-erased resources. ``Resource`` is
an owning type-erased resource: copies share ownership of its stored concrete
resource. ``ResourceRef`` and ``ResourceConstRef`` are mutable and const
non-owning type-erased views. The corresponding event types are ``Event``,
``EventRef``, and ``EventConstRef``.

A concrete resource is created with:

.. code-block:: cpp

   camp::resources::Host h1;
   camp::resources::Cuda c1;

This will create a Host resource and a CUDA resource. With either ``c1`` or ``h1`` we can call different methods 
like ``get_platform()`` or ``get_stream()``. See the Doxygen information for more details. On the other hand, a generic 
resource is created with:

.. code-block:: cpp

   camp::resources::Resource h{h1};
   camp::resources::Resource r{c1};

These ``Resource`` objects own copies of ``h1`` and ``c1``; they do not borrow
them. Constructing from a temporary instead moves the concrete resource into
the owning ``Resource``:

.. code-block:: cpp

   camp::resources::Resource h{camp::resources::Host()};
   camp::resources::Resource r{camp::resources::Cuda()};

Use a concrete resource when code requires a specific resource type. Use a
type-erased resource when code needs one interface for different resource
types.

Borrowing Resources and Events
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``ResourceRef`` provides generic access to a resource without taking
ownership. Creating or copying one does not allocate or participate in shared
ownership. Concrete resources and owning ``Resource`` objects convert
implicitly to ``ResourceRef``, making it useful for short-lived generic
access:

.. code-block:: cpp

   void initialize(camp::resources::ResourceRef resource)
   {
     resource.wait();
   }

   camp::resources::Cuda cuda;
   initialize(cuda);

   camp::resources::Resource owned{camp::resources::Cuda{}};
   initialize(owned);

``ResourceConstRef`` is the const counterpart to ``ResourceRef``. ``EventRef``
and ``EventConstRef`` provide the corresponding non-owning views for events.

Use ``Resource`` or ``Event`` when generic code needs an owning type-erased
object instead of a non-owning view. If the code receives a borrowed resource,
use ``Resource::from_ref`` to store an owning ``Resource`` for later use:

.. code-block:: cpp

   struct PendingWork {
     explicit PendingWork(camp::resources::ResourceRef resource)
         : resource_(camp::resources::Resource::from_ref(resource))
     {}

     camp::resources::Resource resource_;
   };

``from_ref`` returns a ``Resource`` containing a copy of the referenced
concrete resource. The returned ``Resource`` owns that copy rather than
borrowing it through the ``ResourceRef``.

.. important::

   ``ResourceRef``, ``ResourceConstRef``, ``EventRef``, and ``EventConstRef``
   are non-owning. A concrete resource or event must outlive every view that
   refers to it. When a view is created from a ``Resource`` or ``Event``, that
   owner or one of its copies must outlive the view. Views cannot refer to
   temporaries.

Using Resources
~~~~~~~~~~~~~~~

Camp resources provide a common interface for coordinating streams of
execution across supported backends. The following sections show how to use
events, compare and hash resources, and clean up Camp-managed backend state.
See :ref:`using_camp-label` for more examples.

Using Events
^^^^^^^^^^^^

Camp resources allow users a hardware-agnostic way of interacting with the underlying hardware with events. 

.. note::

  Camp events behave just like a normal CUDA/HIP/SYCL events "under the hood", so be sure to refer to the vendor documentation 
  for implementation-specific details.

For example, using a Camp resource, users can create an event with:

.. code-block:: cpp

   camp::resources::Cuda c1;
   c1.get_event();

The ``get_event()`` method will create and record a CUDA event. From here, users can check the event:

.. code-block:: cpp

   if(c1.get_event().check()) {
     // If we get here, the event has completed and check() has returned cudaSuccess
   }

Or explicitly wait on the event:

.. code-block:: cpp

   c1.get_event().wait(); //Explicitly wait for the event to complete
   // Do some work

Users can also use events to synchronize on the device:

.. code-block:: cpp

   #if defined(ENABLE_CUDA)
     using resource_type = camp::resources::Cuda; // Create the (Concrete) Camp resource
   #elif defined(ENABLE_HIP)
     using resource_type = camp::resources::Hip; // Create the (Concrete) Camp resource
   #endif

   ...
   auto resource = camp::resources::Resource{resource_type{}}; // Create a (Generic) Camp resource 
   my_kernel<<<NUM_BLOCKS, THREADS_PER_BLOCK, 0, resource.get_stream()>>>(my_data); // Do some work on the device
   resource.get_event().wait(); // Use the resource to synchronize the device after the kernel
   ...

Using EventProxy
""""""""""""""""

Sometimes a user may want to avoid the overhead of creating an event unless absolutely necessary. In this case,
Camp provides the ``EventProxy`` class. If a piece of code returns an ``EventProxy``, an event is only created if the
user stores the return value as an event. For example:

.. code-block:: cpp

   EventProxy do_something();

   do_something(); // return value ignored no event created

   Event e = do_something(); // event created
   e.wait();

Comparing Resources
^^^^^^^^^^^^^^^^^^^

It may be handy to be able to compare two different resources to see if they are the same or not.
One common use case is when dealing with two different device streams where each stream corresponds
to a separate Camp resource.

.. code-block:: cpp

   camp::resources::Cuda c1, c2; // Create two different Cuda resources
   ...  
   my_kernel<<<NUM_BLOCKS, THREADS_PER_BLOCK, 0, c1.get_stream()>>>(my_data);
   if(c1 != c2) { // Compare device resources
     c1.get_event().wait(); // Synchronize streams if resources are not the same
   }
   my_other_kernel<<<NUM_BLOCKS, THREADS_PER_BLOCK, 0, c2.get_stream()>>>(my_data);
   ...


Owning resources, borrowed resources, and concrete resources can be compared
directly. The underlying concrete types must match to compare equal:

.. code-block:: cpp

   camp::resources::Cuda c1; //This is a typed resource
   camp::resources::Resource other_res = get_other_resource(...); //This is my generic resource
   
   if (other_res == c1) {
     std::cout << "They are equal!" << std::endl;
   }

The same interoperability applies to resource and event views. Equal objects
produce matching hashes across their owning and non-owning forms.

While it is possible for two device resources to be different since each resource refers to a different
device stream, all ``Host`` Camp resources will be the same since there is only one `stream of execution` 
for the Host.

Resource Hashes
^^^^^^^^^^^^^^^

Resource hashes make using resources as keys in data structures like unordered maps possible. Each resource corresponding to 
a unique device stream or queue will get a hash. However, since all Host resources will be the same (because there is only 
one `stream of execution` on the Host), users should be cautious when using the Host resource as a key. For example, 
``std::unordered_map<Host, Value> map`` would only ever have one entry.

Cleaning Up Resources
^^^^^^^^^^^^^^^^^^^^^

Some Camp resources can have global states. These are typically used
for storing streams/queues in the CUDA, HIP and Sycl backends, which are not
automatically deleted on exit. To delete these global states, there
is a ``cleanup`` function for each backend. Resources that do not
store any global state will have no-ops for their respective ``cleanup``
function. Additionally, there is a ``camp::resources::cleanup()`` that
will delete the Camp-managed global state for every enabled backend.

Calling ``cleanup`` function will invalidate existing resource objects that
rely on the global states. If a new resource object is created after 
the ``cleanup`` function is called, then the Camp-managed global state
is re-initialized. For applications, the recommendation is to call 
``camp::resources::cleanup()`` before exiting from ``main``.

.. note::
   The ``cleanup()`` functions are not thread-safe.

.. note::
   The Sycl backend's ``cleanup`` function is currently a no-op and 
   does not delete the Camp-managed global state.
