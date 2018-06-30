// Example-3:
// launch tasks in sequence 

#define PX_SCHED_IMPLEMENTATION 1
#include "../px_sched.h"

int main(int, char **) {
  px_sched::Scheduler schd;
  schd.init();

  px_sched::Sync s;
  for(size_t i = 0; i < 128; ++i) {
    auto job = [i] {
      printf("Task %zu completed from %s\n",
       i, px_sched::Scheduler::current_thread_name());
    };
    px_sched::Sync new_s;
    // run job after s
    schd.runAfter(s, job, &new_s);
    // sync objects can be copied, store the new sync object for the next task
    s = new_s;
  }

  printf("Waiting for tasks to finish...\n");
  schd.waitFor(s); // wait for all tasks to finish
  printf("Waiting for tasks to finish...DONE \n");

  return 0;
}
