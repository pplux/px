// Example-7:
// Multiple Readers, Single Writer pattern

#include <cstdlib> // demo: rand 
#include <mutex> // for the scoped lock

//#define PX_SCHED_CONFIG_SINGLE_THREAD 1
#define PX_SCHED_IMPLEMENTATION 1
#include "../px_sched.h"
#include "common/mem_check.h"


template<class T>
class MRSW {
public:
  ~MRSW() { finish(); }

  void init(px_sched::Scheduler *s) {
    finish();
    sched_ = s;
    void *ptr = sched_->params().mem_callbacks.alloc_fn(alignof(T), sizeof(T));
    obj_ = new (ptr) T();
    read_mode_ = true;
  }

  void finish() {
    if (obj_) {
      sched_->waitFor(next_);
      obj_->~T();
      sched_->params().mem_callbacks.free_fn(obj_);
      obj_ = nullptr;
      sched_ = nullptr;
    }
  }

  template<class T_func>
  void executeRead(T_func func, px_sched::Sync *finish_signal = nullptr) {
    std::lock_guard<px_sched::Spinlock> g(lock_);
    if (!read_mode_) {
      read_mode_ = true;
      prev_ = next_;
      next_ = px_sched::Sync();
    }
    if (finish_signal) sched_->incrementSync(finish_signal);
    sched_->runAfter(prev_, [this, func, finish_signal]() {
      func(static_cast<const T*>(obj_));
      if (finish_signal) sched_->decrementSync(finish_signal);
    }, &next_);
  }

  template<class T_func>
  void executeWrite(T_func func, px_sched::Sync *finish_signal = nullptr) {
    std::lock_guard<px_sched::Spinlock> g(lock_);
    read_mode_ = false;
    px_sched::Sync new_next;
    if (finish_signal) sched_->incrementSync(finish_signal);
    sched_->runAfter(next_, [this, func, finish_signal]() {
      func(obj_);
      if (finish_signal) sched_->decrementSync(finish_signal);
    }, &new_next);
    next_ = new_next;
  }
private:
  px_sched::Scheduler *sched_ = nullptr;
  T *obj_ = nullptr;
  px_sched::Sync prev_;
  px_sched::Sync next_;
  px_sched::Spinlock lock_;
  bool read_mode_;
};

struct Example {
  mutable std::atomic<int32_t> readers = {0};
  std::atomic<int32_t> writers = {0};
};

int main(int, char **) {
  atexit(mem_report);
  px_sched::Scheduler sched_;
  px_sched::SchedulerParams s_params;
  s_params.max_number_tasks = 8196;
  s_params.mem_callbacks.alloc_fn = mem_check_alloc;
  s_params.mem_callbacks.free_fn = mem_check_free;
  sched_.init(s_params);

  MRSW<Example> example;
  example.init(&sched_);

  for(uint32_t i = 0; i < 1000; ++i) {
    if ((std::rand() & 0xFF) < 200) {
      example.executeRead([i](const Example *e) {
        e->readers.fetch_add(1);
        printf("[%u] Read Op  %d(R)/%d(W)\n", i, e->readers.load(), e->writers.load());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        e->readers.fetch_sub(1);
      });
    } else {
      example.executeWrite([i](Example *e) {
        e->writers.fetch_add(1);
        printf("[%u] Write Op %d(R)/%d(W)\n", i, e->readers.load(), e->writers.load());
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        e->writers.fetch_sub(1);
      });
    }
  }

  printf("WAITING FOR TASKS TO FINISH....\n");


  return 0;
}
