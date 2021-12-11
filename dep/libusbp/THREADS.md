# Thread safety

If you are using multiple threads in your application, it is important to make sure that your calls to libusbp will be thread safe.  This page covers everything you need to know about thread safety and libusbp.

We will only discuss the C API functions defined in libusbp.h.  The C++ API defined in libusbp.hpp is just a simple wrapper around the C API and does not introduce or solve any thread safety issues.

This library does not create threads, use mutable global variables, use volatile variables, use mutexes, use memory barriers, or use reference counting.

On this page, two function calls are said to *conflict* with each other if there is no guarantee that executing the function calls concurrently on different threads will work as expected.  To characterize the thread-safety of libusbp, we will specify which pairs of function calls conflict with each other.  A function call consists of the name of a library function being called along with the values of its arguments.

Two function calls will not conflict if the memory areas pointed to by their arguments have no overlap.  This is because the library does not use any mutable global variables.  To determine whether the memory areas overlap, you will need to know which library objects hold pointers to other library objects.  The rules for that are defined in the "Pointer rules" section below.

If there is an overlap in the memory areas pointed to by the arguments of two functions calls, the overlap will not cause a conflict as long as all of the parameters that are responsible for the overlap are marked with the `const` qualifier in the header.  We use `const` as an indicator that the function will not modify the memory pointed to by that argument, and it will not call any API functions that might change the state of the underlying handles held by the object.

## Pointer rules

* Each ::libusbp_async_in_pipe object may hold a pointer to the ::libusbp_generic_handle that it was created from.  Similarly, the ::libusbp_generic_handle may hold pointers to its ::libusbp_async_in_pipe objects.
* All other objects contain no pointers to each other.
