Disclaimer
==========

This collection of c++11 utilities is here so that I can conveniently add it as a sub-module for my
other projects. It's probably full of bugs, unoptimized and maybe contains horrifying coding abdominations.

So: Use at your own risk :)

Content
=======

Each of the this utility is commented, but following is a small overview of each one. For more information,
check the comments directly in the sources.

Memory Tracker
--------------

This one allows me to ensure that I'm not leaking memory. It relies on using a macro to allocate things,
so it's probably tedious to use on an existing codebase, but using it from the start on a small project,
it's a lifesaver.

How to use:

```
// in a header:
#define MEMORY_CHECK 1
#include "MemoryTracker.h"

// in a compilation unit:
#define MEMORY_TRACKER_IMPLEMENTATION
#include "MemoryTracker.h"

// in the rest of the code, use the provided macros to (de)allocate memory:
char * data = MT_NEW char[32];
MT_DELETE [] data;

// you can managed the tracked memory using the following macros:

// this one reset the memory tracker
MT_INIT_MEM_TRACKER();

// this one will make the app crash on the 124'th allocation
MT_BREAK_ON_ALLOC(124);

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

```
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

```
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
