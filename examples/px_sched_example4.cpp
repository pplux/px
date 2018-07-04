// Example-4:
// Tasks can launch sub-tasks and wait for them

#define PX_SCHED_IMPLEMENTATION 1
#include "../px_sched.h"
#include "common/mem_check.h"

int main(int, char **) {
  atexit(mem_report);
  px_sched::Scheduler schd;
  px_sched::SchedulerParams s_params;
  s_params.mem_callbacks.alloc_fn = mem_check_alloc;
  s_params.mem_callbacks.free_fn = mem_check_free;
  schd.init(s_params);

  px_sched::Sync s;
  for(size_t i = 0; i < 10; ++i) {
    auto job = [i] {
      printf("Phase 1: Task %zu completed from %s\n",
       i, px_sched::Scheduler::current_thread_name());
    };
    schd.run(job, &s);
  }

  px_sched::Sync last;
  schd.runAfter(s, [&schd]{
    printf("Phase 2\n");
    px_sched::Sync s2;
    for(size_t i = 0; i < 10; ++i) {
      auto job = [i] {
        printf("Phase 2: Task %zu completed from %s\n",
         i, px_sched::Scheduler::current_thread_name());
      };
      schd.run(job, &s2);
    }
    schd.waitFor(s2);
    printf("Phase 2, done\n");
   }, &last);

  printf("Waiting for tasks to finish...\n");
  schd.waitFor(last); // wait for all tasks to finish
  printf("Waiting for tasks to finish...DONE \n");

  return 0;
}
