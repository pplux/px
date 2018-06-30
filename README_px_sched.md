[![Build Status](https://travis-ci.org/pplux/px_sched.svg?branch=master)](https://travis-ci.org/pplux/px_sched)
[![Build Status AppVeyor](https://ci.appveyor.com/api/projects/status/github/pplux/px_sched)](https://ci.appveyor.com/project/pplux/px-sched)

# px_sched
Single Header C++ Task Scheduler 

Written in C++11(only for thread API), with no dependency, and easy to integrate. See the examples: 
[ex1.cpp](https://github.com/pplux/px/blob/master/examples/px_sched_example1.cpp),
[ex2.cpp](https://github.com/pplux/px/blob/master/examples/px_sched_example2.cpp),
[ex3.cpp](https://github.com/pplux/px/blob/master/examples/px_sched_example3.cpp),
[ex4.cpp](https://github.com/pplux/px/blob/master/examples/px_sched_example4.cpp),
[ex5.cpp](https://github.com/pplux/px/blob/master/examples/px_sched_example5.cpp),
[ex6.cpp](https://github.com/pplux/px/blob/master/examples/px_sched_example6.cpp),
[ex7.cpp](https://github.com/pplux/px/blob/master/examples/px_sched_example7.cpp).



## Goals:
* Allow task oriented multihtread programmming without mutexes, locks, condition variables...
* Implicit task graph dependency for single or groups of tasks
* Portable, written in C++11 with no dependency
* C++11 only used for thread, mutex, and condition variable *no STL container is used*
* Easy to use, flexible, and lightweight
* Inspied by Naughty Dog's talk [Parallelizing the Naughty Dog Engine](https://www.gdcvault.com/play/1022186/Parallelizing-the-Naughty-Dog-Engine), [enkiTS](https://github.com/dougbinks/enkiTS), and [STB's single-file libraries](https://github.com/nothings/stb)
* No dynamic memory allocations, all memory is allocated at [initialization](https://github.com/pplux/px_sched/blob/4b84af4a1ffb1a06bbf70f3d70301ca26357fba8/px_sched.h#L112). Easy to integrate with your own memory manager if needed.

## API:

```cpp
int main(int, char **) {
  px::Scheduler schd;
  schd.init();

  px::Sync s1,s2,s3;
  
  // First we launch a group of 10 concurrent tasks, all of them referencing the same sync object (s1)
  for(size_t i = 0; i < 10; ++i) {
    auto job = [i] {
      printf("Phase 1: Task %zu completed from %s\n",
       i, px::Scheduler::current_thread_name());
    };
    schd.run(job, &s1);
  }

  // Another set of 10 concurrent tasks will be launched, once the first group (s1) finishes.
  // The second group will be attached to the second Sync object (s2).
  for(size_t i = 0; i < 10; ++i) {
    auto job = [i] {
      printf("Phase 2: Task %zu completed from %s\n",
       i, px::Scheduler::current_thread_name());
    };
    schd.runAfter(s1, job, &s2);
  }

  // Finally another group, waiting for the second (s2) fo finish. 
  for(size_t i = 0; i < 10; ++i) {
    auto job = [i] {
      printf("Phase 3: Task %zu completed from %s\n",
       i, px::Scheduler::current_thread_name());
    };
    schd.runAfter(s2, job, &s3);
  }

  // Sync objects can be copied, internally they are just a handle.
  px::Sync last = s3;
  
  // The main thread will now wait for the last task to finish, the task graph is infered by the 
  // usage of sync objects and no wait has ocurred on the main thread until now. 
  printf("Waiting for tasks to finish...\n");
  schd.waitFor(last); // wait for all tasks to finish
  printf("Waiting for tasks to finish...DONE \n");

  return 0;
}
```

`px::Scheduler` is the main object, normally you should only instance one, but
that's up to you. There are two methods to launch tasks:

* `run(const px::Job &job, px::Sync *optional_sync_object = nullptr)`
* `runAfter(const px::Sync trigger, const px::Job &job, px::Sync *out_optional_sync_obj = nullptr)`

Both run methods receive a `Job` object, by default it is a `std::function<void()>` but you can [customize](https://github.com/pplux/px_sched/blob/master/examples/example2.cpp) to fit your needs. 

Both run methods receive an optional output argument, a `Sync` object. `Sync` objects are used to coordinate dependencies between groups of tasks (or single tasks). The simplest case is to wait for a group of tasks to finish:

```cpp
px::Sync s;
for(size_t i = 0; i < 128; ++i) {
  schd.run([i]{printf("Task %zu\n",i);}, &s);
}
schd.waitFor(s);
```

`Sync` objects can also be used to launch tasks when a group of tasks have finished with the method `px::Scheduler::runAfter`:

```cpp
px::Sync s;
for(size_t i = 0; i < 128; ++i) {
  schd.run([i]{printf("Task %zu\n",i);}, &s);
}
px::Sync last;
schd.runAfter(s, []{printf("Last Task\n");}, last);

schd.waitFor(last);
```

## TODO's
* [  ] improve documentation
* [  ] Add support for Windows Fibers on windows
* [  ] Add support for ucontext on Posix
