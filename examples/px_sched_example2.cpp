// Example-2:
// Custom Job Definition

#include <cstdlib>
// Job definition must be done before including px_sched.h 
#define PX_SCHED_CUSTOM_JOB_DEFINITION
namespace px_sched {
  struct Job {
    void (*func)(size_t);
    size_t n;
    void operator()() { func(n); }
  };
} // px namespace

#define PX_SCHED_IMPLEMENTATION 1
#include "../px_sched.h"

void task(size_t n) {
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  printf("Task %zu completed from %s\n", n, px_sched::Scheduler::current_thread_name());
}

int main(int, char **) {
  px_sched::Scheduler schd;
  schd.init();

  // (a) Sync objects can be used later to wait for them
  px_sched::Sync s;
  for(size_t i = 0; i < 128; ++i) {
    px_sched::Job job { task, i };
    // run a task, and notify to the given sync object
    schd.run(std::move(job), &s);
  }

  printf("Waiting for tasks to finish...\n");
  schd.waitFor(s); // wait for all tasks to finish
  printf("Waiting for tasks to finish...DONE \n");

  return 0;
}
