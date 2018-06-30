// Example-1:
// launch N tasks in parallel, wait for all of them to finish

#define PX_SCHED_IMPLEMENTATION 1
#include "../px_sched.h"

int main(int, char **) {
  px_sched::Scheduler schd;
  schd.init();

  // (a) Sync objects can be used later to wait for them
  px_sched::Sync s;
  for(size_t i = 0; i < 128; ++i) {
    auto job = [i] {
      printf("Task %zu completed from %s\n",
       i, px_sched::Scheduler::current_thread_name());
    };
    // run a task, and notify to the given sync object
    schd.run(job, &s);
  }

  printf("Waiting for tasks to finish...\n");
  schd.waitFor(s); // wait for all tasks to finish
  printf("Waiting for tasks to finish...DONE \n");

  return 0;
}
