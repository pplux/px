// Example-8:
// Job order (mainly to check Single Threaded implementation) 

#define PX_SCHED_IMPLEMENTATION 1
#include "../px_sched.h"
#include <cassert>

int main(int, char **) {
  px_sched::Scheduler schd;
  schd.init();

  // (a) Sync objects can be used later to wait for them
  px_sched::Sync start;
  px_sched::Sync middle;
  px_sched::Sync end;
  schd.incrementSync(&start);

  schd.incrementSync(&middle);
  size_t data[128] = {};
  schd.runAfter(middle, [&data]{
    printf("Checking...\n");
    for(size_t i = 0; i < 128; ++i) {
      assert(data[i] == i*2);
    }
  }, &end);
  for(size_t i = 0; i < 128; ++i) {
    schd.runAfter(start,[i, &data] () mutable {
      printf("Running %zu\n",i);
      data[i] = i*2;
    }, &middle);
  }
  schd.decrementSync(&middle);
  assert(data[127] == 0);
  // start now
  schd.decrementSync(&start);
  printf("Waiting for tasks to finish...\n");
  schd.waitFor(end); // wait for all tasks to finish
  assert(data[127] == 254);
  printf("Waiting for tasks to finish...DONE \n");

  return 0;
}
