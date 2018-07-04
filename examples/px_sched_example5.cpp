// Example-5:
// Launch 3 phases, without Waiting and locking

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

  px_sched::Sync s1,s2,s3;
  for(size_t i = 0; i < 10; ++i) {
    auto job = [i] {
      printf("Phase 1: Task %zu completed from %s\n",
       i, px_sched::Scheduler::current_thread_name());
    };
    schd.run(job, &s1);
  }

  for(size_t i = 0; i < 10; ++i) {
    auto job = [i] {
      printf("Phase 2: Task %zu completed from %s\n",
       i, px_sched::Scheduler::current_thread_name());
    };
    schd.runAfter(s1, job, &s2);
  }

  for(size_t i = 0; i < 10; ++i) {
    auto job = [i] {
      printf("Phase 3: Task %zu completed from %s\n",
       i, px_sched::Scheduler::current_thread_name());
    };
    schd.runAfter(s2, job, &s3);
  }


  px_sched::Sync last = s3;
  printf("Waiting for tasks to finish...\n");
  schd.waitFor(last); // wait for all tasks to finish
  printf("Waiting for tasks to finish...DONE \n");

  return 0;
}
