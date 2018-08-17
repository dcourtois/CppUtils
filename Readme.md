Disclaimer
==========

This collection of c++11 utilities is here so that I can conveniently add it as a sub-module for my
other projects. It's probably full of bugs, unoptimized and maybe contains horrifying coding abdominations.

So: Use at your own risk :)

Content
=======

Each of the this utility is commented, but following is a small overview of each one. For more information,
check the comments directly in the sources.

Hash
----

Utilities related to hashing. For the moment:

- [Jenkins' One-at-a-time](https://en.wikipedia.org/wiki/Jenkins_hash_function#one_at_a_time) algorithm to hash strings or arbitrary char data
- Compile-time implementation of the previous algorithm
- Combine hash keys

How to use:

```cpp
#include "Hash.h"

// somewhere, you have a string and want to check its various possible values. Thanks
// to compile time hashing, you can use a switch (assuming str is a std::string)
switch (Hash::Jenkins(str))
{
	case HASH("foo"):
		break;

	case HASH("bar"):
		break;

	// ...
}

// if you want to combine 2 hashes:
uint32_t key1 = Hash::Jenkins("foo");
uint32_t key2 = Hash::Jenkins("bar");
uint32_t key = Hash::Combine(key1, key2);
```

Memory Tracker
--------------

This one allows me to ensure that I'm not leaking memory. It relies on using a macro to allocate things,
so it's probably tedious to use on an existing codebase, but using it from the start on a small project,
it's a lifesaver.

How to use:

```cpp
// in a header:
#define MEMORY_CHECK 1
#include "MemoryTracker.h"

// in a compilation unit:
#define MEMORY_TRACKER_IMPLEMENTATION
#include "MemoryTracker.h"

// by default, the global memory tracker is disabled (to avoid tracking statically
// allcoated data) so you need to enable it at the start of your app, for instance
// at the beginning of the main
MT_SET_ENABLED(true);

// in the rest of the code, use the provided macros to (de)allocate memory:
char * data = MT_NEW char[32];
MT_DELETE [] data;

// you can manage the tracked memory using the following macros:

// this one reset the memory tracker
MT_INIT_MEM_TRACKER();

// this one will make the app crash on the 124'th allocation
MT_BREAK_ON_ALLOCATION(124);

// get the currently tracked memory
int bytes = MT_GET_ALLOCATED_MEM();

// print to the stdout a report of the currently tracked memory
MT_DUMP_LEAKS();
```

Profiler
--------

Provides timing utilities, and macros used to automatically profile functions, scopes or
code, etc. It also provides a way to output the profiled data to a json file that can then
be displayed using Chrome's tracing functionality.

How to use:

```cpp
// in a header
#define PROFILER_ENABLE 1
#include "Profiler.h"

// in a compilation unit
#define PROFILER_IMPLEMENTATION
#include "Profiler.h"

// in the rest of the code
void SomeFunction(void)
{
	// will profile the whole function
	PROFILE_FUNCTION();

	{
		// profile base on the scope this macro is used
		PROFILE_SCOPE("SomeFunction - scope foo");
	}
}
```


TaskManager
-----------

Probably badly named, but this class just create N threads, and allow you to push jobs to it. As soon
as a thread is available, it will process the job in a FIFO manner.

It also has a sort of builtin thread local storage (basically a `void *` data per thread that will be
passed to the jobs)

How to use:

```cpp
#include "TaskManager.h"


// the parameter is the number of threads. If omitted, it's automatically taken from the current hardware.
TaskManager taskManager(4);

// you can optionally setup local storage. Be carefull about the lifetime of the data: it must be at least
// the same as the task manager's.
SomeData tls[4];
for (int i = 0; i < 4; ++i)
{
	taskManager.SetThreadLocalStorage(i, &tls);
}

// push some jobs
for (int i = 0; i < 10; ++i)
{
	taskManager.PushTask([] (void *) {
		// do some meaning full multithreaded things
	});
}
```


STLUtils
--------

Just a collection of free functions mostly to work with containers, because I'm lazy, and usually I prefer
writing `if (Contains(container, item) == true)` instead of `if (std::find(container.begin(), container.end(), item) != container.end())`

Variant
-------

Small helper used to store and convert to / from numerical types, boolean and string.

```cpp
// you can initialize from basic types
Variant foo(1.0);

// you can convert to other types
std::string str = static_cast< std::string >(foo);
bool b = static_cast< bool >(foo);

// you can dynamically change the type
foo = "bar";

// and compare
bool baz = foo != "baz";
bool bar = foo == "bar";
```

Settings
--------

This one is a class which handles loading / saving settings, taking care of removing the ones that are no longer accessed.


```cpp
// create the class and load things.
Settings settings("config.bin");
settings.Load();

// load stuff (second argument is a default value used if the setting doesn't exist)
double value = settings.Get("some.double", -1.0);

// write stuff (overwrite existing settings, create new ones)
settings.Set("some.double", 10.0);

// save the updated settings. Note that every setting which hasn't been used (either by Set or Get)
// since the Load call will not be saved. So you might want to create a single global instance for
// you whole app.
settings.Save();

```

PoolAllocator
-------------

Templated class used to create pools of a given type, to avoid allocation costs for small objects, or just to
ensure that objects of the same type are in a contiguous memory space. Note that the pool allocator is using
the memory tracker functionalities to track (de)allocation.

```cpp
PoolAllocator< Foo, 100 > allocator;
Foo * foo = allocator.Allocate();
allocator.Deallocate(foo);
```
