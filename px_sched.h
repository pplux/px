/* -----------------------------------------------------------------------------
Copyright (c) 2017-2018 Jose L. Hidalgo (PpluX)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
----------------------------------------------------------------------------- */

// USAGE
// Include this file in any file it needs to refer to it, but if you need
// to change any define, or use a custom job description is preferable to
// create a header that sets the defines, defines the job description and
// finally includes this file.
//
// In *ONE* C++ file you need to declare
// #define PX_SCHED_IMPLEMENTATION 1
// before including the file that contains px_sched.h

#ifndef PX_SCHED
#define PX_SCHED

// -- Job Object ----------------------------------------------------------
// if you want to use your own job objects, define the following
// macro and provide a struct px_sched::Job object with an operator() method.
//
// Example, a C-like pointer to function:
//
//    #define PX_SCHED_CUSTOM_JOB_DEFINITION
//    namespace px_sched {
//      struct Job {
//        void (*func)(void *arg);
//        void *arg;
//        void operator()() { func(arg); }
//      };
//    } // px namespace
//
//  By default Jobs are simply std::function<void()>
//
#ifndef PX_SCHED_CUSTOM_JOB_DEFINITION
#include <functional>
namespace px_sched {
  typedef std::function<void()> Job;
} // px namespace
#endif
// -----------------------------------------------------------------------------

// -- Backend selection --------------------------------------------------------
// Right now there is only two backends(single-threaded, and regular threads),
// in the future we will add windows-fibers and posix-ucontext. Meanwhile try
// to avoid waitFor(...) and use more runAfter if possible. Try not to suspend
// threads on external mutexes.
#if !defined(PX_SCHED_CONFIG_SINGLE_THREAD)  && \
    !defined(PX_SCHED_CONFIG_REGULAR_THREADS)
# define PX_SCHED_CONFIG_REGULAR_THREADS 1
#endif

#ifdef PX_SCHED_CONFIG_REGULAR_THREADS
#  define PX_SCHED_IMP_REGULAR_THREADS 1
#else
#  define PX_SCHED_IMP_REGULAR_THREADS 0
#endif

#ifdef PX_SCHED_CONFIG_SINGLE_THREAD
#  define PX_SCHED_IMP_SINGLE_THREAD 1
#else
#  define PX_SCHED_IMP_SINGLE_THREAD 0
#endif

#if ( PX_SCHED_IMP_SINGLE_THREAD   \
    + PX_SCHED_IMP_REGULAR_THREADS \
    ) != 1
#error "PX_SCHED: Only one backend must be enabled (and at least one)"
#endif
// -----------------------------------------------------------------------------

#ifndef PX_SCHED_CACHE_LINE_SIZE
#define PX_SCHED_CACHE_LINE_SIZE 64
#endif

// **WARNING** ---> WORK IN PROGRESS <--- **WARNING** 
// Enable if you want the threads to track resource locking, this might slow
// down things a bit.
#ifndef PX_SCHED_CHECK_DEADLOCKS
#define PX_SCHED_CHECK_DEADLOCKS 0
#endif
// -----------------------------------------------------------------------------


// some checks, can be omitted if you're confident there is no
// misuse of the library. 
#ifndef PX_SCHED_DOES_CHECKS
#define PX_SCHED_DOES_CHECKS 1
#endif

// PX_SCHED_CHECK_FN -----------------------------------------------------------
// used to check conditions, to test for erros
// defined only if PX_SCHED_DOES_CHECKS is evaluated to true. By 
// default uses printf and abort, use your own implementation by defining
// PX_SCHED_CHECK_FN before including this header.
#ifndef PX_SCHED_CHECK_FN
#  if PX_SCHED_DOES_CHECKS
#    include <stdlib.h>
#    include <stdio.h>
#    define PX_SCHED_CHECK_FN(cond, ...) \
        if (!(cond)) { \
          printf("-- PX_SCHED ERROR: -------------------\n"); \
          printf("-- %s:%d\n", __FILE__, __LINE__); \
          printf("--------------------------------------\n"); \
          printf(__VA_ARGS__);\
          printf("\n--------------------------------------\n"); \
          abort(); \
        } 
#  else
#    define PX_SCHED_CHECK_FN(...) /*NO CHECK*/
#  endif // PX_SCHED_DOES_CHECKS
#endif // PX_SCHED_CHECK_FN

#include <atomic>
#include <condition_variable>
#include <thread>

namespace px_sched {

  // Sync object
  class Sync {
    uint32_t hnd = 0;
    friend class Scheduler;
  };


  struct MemCallbacks {
    void* (*alloc_fn)(size_t amount) = ::malloc;
    void (*free_fn)(void *ptr) = ::free;
  };

  struct SchedulerParams {
    uint16_t num_threads = 16;        // num OS threads created 
    uint16_t max_running_threads = 0; // 0 --> will be set to max hardware concurrency
    uint16_t max_number_tasks = 1024; // max number of simultaneous tasks
    uint16_t thread_num_tries_on_idle = 16;   // number of tries before suspend the thread
    uint32_t thread_sleep_on_idle_in_microseconds = 5; // time spent waiting between tries
    MemCallbacks mem_callbacks;
  };

  // -- Atomic -----------------------------------------------------------------
  template<class T>
  struct Atomic {
  #if PX_SCHED_IMP_SINGLE_THREAD
    T data = {};
    Atomic() = default;
    ~Atomic() = default;
    explicit Atomic(const T&d) : data(d) {}
    bool compare_exchange_strong(T &current, const T &next) {
        if (data == current) {
            data = next;
            return true;
        }
        current = data;
        return false;
    }
    bool compare_exchange_weak(T &current, const T &next) {
        return compare_exchange_strong(current,next);
    }
    const T load() const { return data; }
    void store(const T& v) { data = v; }
    T fetch_add(const T&v) {
        T fetch(data);
        data += v;
        return fetch;
    }
    T fetch_sub(const T&v) {
        T fetch(data);
        data -= v;
        return fetch;
    }
    T exchange(const T&v) {
        T old(data);
        data = v;
        return old;
    }
    bool operator==(const Atomic &other) const { return other.data == data; }
    bool operator==(const T &other) const { return other== data; }
  #else
    std::atomic<T> data = {};
    Atomic() = default;
    ~Atomic() = default;
    explicit Atomic(const T&d) : data(d) {}
    bool compare_exchange_strong(T &current, const T &next) {
        return data.compare_exchange_strong(current, next);
    }
    bool compare_exchange_weak(T &current, const T &next) {
        return data.compare_exchange_weak(current, next);
    }
    const T load() const { return data.load(); }
    void store(const T& v) { data = v; }
    T fetch_add(const T&v) { return data.fetch_add(v); }
    T fetch_sub(const T&v) { return data.fetch_sub(v); }
    T exchange(const T&v) { return data.exchange(v); }
    bool operator==(const Atomic &other) const { return other.data == data; }
    bool operator==(const T &other) const { return other== data; }
  #endif
  };


  // -- ObjectPool -------------------------------------------------------------
  // holds up to 2^20 objects with ref counting and versioning
  // used internally by the Scheduler for tasks and counters, but can also
  // be used as a thread-safe object pool

  template<class T>
  struct ObjectPool {
    const uint32_t kPosMask = 0x000FFFFF; // 20 bits
    const uint32_t kRefMask = kPosMask;   // 20 bits
    const uint32_t kVerMask = 0xFFF00000; // 12 bits
    const uint32_t kVerDisp = 20;

    ~ObjectPool();

    void init(uint32_t count, const MemCallbacks &mem = MemCallbacks());
    void reset();

    // only access objects you've previously referenced
    T& get(uint32_t hnd);

    // only access objects you've previously referenced
    const T& get(uint32_t hnd) const;

    // given a position inside the pool returns the handler and also current ref
    // count and current version number (only used for debugging)
    uint32_t info(size_t pos, uint32_t *count, uint32_t *ver) const;

    // max number of elements hold by the object pool
    uint32_t size() const { return count_; }

    // returns the handler of an object in the pool that can be used
    // it also increments in one the number of references (no need to call ref)
    uint32_t adquireAndRef();

    void unref(uint32_t hnd) const;

    // decrements the counter, if the object is no longer valid (last ref)
    // the given function will be executed with the element
    template<class F>
    void unref(uint32_t hnd, F f) const;

    // returns true if the given position was a valid object
    bool ref(uint32_t hnd) const;

    uint32_t refCount(uint32_t hnd) const;

  private:
    void newElement(uint32_t pos) const;
    void deleteElement(uint32_t pos) const;

    struct D {
      mutable Atomic<uint32_t> state;
      uint32_t version = 0;
      T element;
#if PX_SCHED_CACHE_LINE_SIZE
      // Avoid false sharing between threads
      static const size_t PADDING_ADJUSTMENT =
          ( PX_SCHED_CACHE_LINE_SIZE
            - ((sizeof(state)+sizeof(version)+sizeof(element))%PX_SCHED_CACHE_LINE_SIZE)
          ) % PX_SCHED_CACHE_LINE_SIZE;
      char padding[PADDING_ADJUSTMENT];
#endif
    }; // D struct

    Atomic<uint32_t> next_;
    D *data_ = nullptr;
    size_t count_ = 0;
    MemCallbacks mem_;
  };


  class Scheduler {
  public:
    Scheduler();
    ~Scheduler();

    void init(const SchedulerParams &params = SchedulerParams());
    void stop();

    void run(const Job &job, Sync *out_sync_obj = nullptr);
    void runAfter(Sync sync,const Job &job, Sync *out_sync_obj = nullptr);
    void waitFor(Sync sync); //< suspend current thread 

    // returns the number of tasks not yet finished associated to the sync object
    // thus 0 means all of them has finished (or the sync object was empty, or
    // unused)
    uint32_t numPendingTasks(Sync s);

    bool hasFinished(Sync s) { return numPendingTasks(s) == 0; }
  
    // Call this only to print the internal state of the scheduler, mainly if it 
    // stops working and want to see who is waiting for what, and so on.
    void getDebugStatus(char *buffer, size_t buffer_size);

    // manually increment the value of a Sync object. Sync objects triggers
    // when they reach 0.
    // *WARNING*: calling increment without a later decrement might leave
    //            tasks waiting forever, and will leak resources.
    void incrementSync(Sync *s);

    // manually decrement the value of a Sync object.
    // *WARNING*: never call decrement on a sync object wihtout calling 
    //            @incrementSync first.
    void decrementSync(Sync *s);

    // By default workers will be named as Worker-id
    // The pointer passed here must be valid until set_current_thread is called
    // again...
    static void set_current_thread_name(const char *name);
    static const char *current_thread_name();

    // Call this method before a mutex/lock/etc... to notify the scheduler
    static void CurrentThreadSleeps();

    // call this again to notify the thread is again running
    static void CurrentThreadWakesUp();

    // Call this method before locking a resource, this will be used by the
    // scheduler to wakeup another thread as a worker, and also can be used
    // later to detect deadlocks (if compiled with PX_SCHED_CHECK_DEADLOCKS 1,
    // WIP!)
    static void CurrentThreadBeforeLockResource(const void *resource_ptr, const char *name = nullptr);

    // Call this method after calling CurrentThreadBeforeLockResource, this will be
    // used to notify the scheduler that this thread can continue working.
    // If success is true, the lock was successful, false if the thread was not
    // blocked but also didn't adquired the lock (try_lock)
    static void CurrentThreadAfterLockResource(bool success);

    // Call this method once the resouce is unlocked.
    static void CurrentThreadReleasesResource(const void *resource_ptr);

    const SchedulerParams& params() const { return params_; }

    // Number of active threads (executing tasks)
    uint32_t active_threads() const { return active_threads_.load(); }

  private:
    struct TLS;
    static TLS* tls();
    void wakeUpOneThread();
    SchedulerParams params_;
    Atomic<uint32_t> active_threads_;
    Atomic<uint32_t> running_;

    struct WaitFor;

    struct Task {
      Job job;
      uint32_t counter_id = 0;
      Atomic<uint32_t> next_sibling_task;
    };

    struct Counter {
      Atomic<uint32_t> task_id;
      Atomic<uint32_t> user_count;
      WaitFor *wait_ptr = nullptr;
    };

    ObjectPool<Task> tasks_;
    ObjectPool<Counter> counters_;
    uint32_t createTask(const Job &job, Sync *out_sync_obj);
    uint32_t createCounter();
    void unrefCounter(uint32_t counter_hnd);

#if PX_SCHED_IMP_REGULAR_THREADS
    struct IndexQueue {
      ~IndexQueue() {
        PX_SCHED_CHECK_FN(list_ == nullptr, "IndexQueue Resources leaked...");
      }
      void reset() {
        if (list_) {
          mem_.free_fn(list_);
          list_ = nullptr;
        }
        size_ = 0;
        in_use_ = 0;
      }
      void init(uint16_t max, const MemCallbacks &mem_cb = MemCallbacks()) {
        _lock();
        reset();
        mem_ = mem_cb;
        size_ = max;
        in_use_ = 0;
        list_ = static_cast<uint32_t*>(mem_.alloc_fn(sizeof(uint32_t)*size_));
        _unlock();
      }
      void push(uint32_t p) {
        _lock();
        PX_SCHED_CHECK_FN(in_use_ < size_, "IndexQueue Overflow total in use %hu (max %hu)", in_use_, size_);
        uint16_t pos = (current_ + in_use_)%size_;
        list_[pos] = p;
        in_use_++;
        _unlock();
      }
      uint16_t in_use() {
        _lock();
        uint16_t result = in_use_;
        _unlock();
        return result;
      }
      bool pop(uint32_t *res) {
        _lock();
        bool result = false;
        if (in_use_) {
          if (res) *res = list_[current_];
          current_ = (current_+1)%size_;
          in_use_--;
          result = true;
        }
        _unlock();
        return result;
      }
      void _unlock() { lock_.clear(std::memory_order_release); }
      void _lock() {
        while(lock_.test_and_set(std::memory_order_acquire)) {
          std::this_thread::yield();
        }
      }
      uint32_t *list_ = nullptr;
      std::atomic_flag lock_ = ATOMIC_FLAG_INIT;
      MemCallbacks mem_;
      volatile uint16_t size_ = 0;
      volatile uint16_t in_use_ = 0;
      volatile uint16_t current_ = 0;
    };

    struct WaitFor {
      explicit WaitFor() 
        : owner(std::this_thread::get_id())
        , ready(false) {}
      void wait() {
        PX_SCHED_CHECK_FN(std::this_thread::get_id() == owner,
            "WaitFor::wait can only be invoked from the thread "
            "that created the object");
        std::unique_lock<std::mutex> lk(mutex);
        if(!ready) {
          condition_variable.wait(lk);
        }
      }
      void signal() {
        if (owner != std::this_thread::get_id()) {
          std::lock_guard<std::mutex> lk(mutex);
          ready = true;
          condition_variable.notify_all();
        } else {
          ready = true;
        }
      }
    private:
      std::thread::id const owner;
      std::mutex mutex;
      std::condition_variable condition_variable;
      bool ready;
    };

    struct Worker {
      std::thread thread;
       // setted by the thread when is sleep
      Atomic<WaitFor*> wake_up;
      TLS *thread_tls = nullptr;
      uint16_t thread_index = 0xFFFF;
    };

    uint16_t wakeUpThreads(uint16_t max_num_threads);

    Worker *workers_ = nullptr;
    IndexQueue ready_tasks_;

    static void WorkerThreadMain(Scheduler *schd, Worker *);
#endif 


  };

  //-- Optional: Mutex template to encapsultae scheduler notification ----------
  template<class M>
  class Mutex {
  public:

    ~Mutex() { lock(); }

    void lock() {
      std::thread::id tid = std::this_thread::get_id();
      if (owner_ == tid) {
        count_++;
        return;
      }
      Scheduler::CurrentThreadBeforeLockResource(&mutex_);
      mutex_.lock();
      owner_ = tid;
      count_ = 1;
      Scheduler::CurrentThreadAfterLockResource(true);
    }
    void unlock() {
      std::thread::id tid = std::this_thread::get_id();
      PX_SCHED_CHECK_FN(owner_ == tid, "Invalid Mutex::unlock owner mistmatch");
      count_--;
      if (count_ == 0) {
        owner_ = std::thread::id();
        Scheduler::CurrentThreadReleasesResource(&mutex_);
        mutex_.unlock();
      }
    }

    bool try_lock() {
      std::thread::id tid = std::this_thread::get_id();
      if (owner_ == tid) {
        count_++;
        return true;
      }
      Scheduler::CurrentThreadBeforeLockResource(&mutex_);
      bool result = mutex_.try_lock();
      if (result) {
        owner_ = tid;
        count_ = 1;
      }
      Scheduler::CurrentThreadAfterLockResource(result);
      return result;
    }
  private:
    std::thread::id owner_ ;
    uint32_t count_ = 0;
    M mutex_;
  };

  //-- Optional: Spinlock ------------------------------------------------------
  class Spinlock {
  public:
    Spinlock() : owner_(std::thread::id()) {}

    ~Spinlock() {
      lock();
    }

    void lock() {
      while(!try_lock()) {}
    }

    void unlock() {
      std::thread::id tid = std::this_thread::get_id();
      PX_SCHED_CHECK_FN(owner_ == tid, "Invalid Spinlock::unlock owner mistmatch");
      count_--;
      if (count_ == 0) {
        owner_.store(std::thread::id());
      }
    }

    bool try_lock() {
      std::thread::id tid = std::this_thread::get_id();
      if (owner_ == tid) {
        count_++;
        return true;
      }
      std::thread::id expected;
      if (owner_.compare_exchange_weak(expected, tid)) {
        count_ = 1;
        return true;
      }
      return false;
    }
  private:
    Atomic<std::thread::id> owner_ ;
    uint32_t count_;
  };

  //-- Object pool implementation ----------------------------------------------
  template<class T>
  inline ObjectPool<T>::~ObjectPool() {
    reset();
  }
  
  template<class T>
  void ObjectPool<T>::newElement(uint32_t pos) const {
    new (&data_[pos].element) T;
  }

  template<class T>
  void ObjectPool<T>::deleteElement(uint32_t pos) const {
    data_[pos].element.~T();
  }

  template<class T>
  inline void ObjectPool<T>::init(uint32_t count, const MemCallbacks &mem_cb) {
    reset();
    mem_ = mem_cb;
    data_ = static_cast<D*>(mem_.alloc_fn(sizeof(D)*count));
    for(uint32_t i = 0; i < count; ++i) {
      data_[i].state.store(0xFFFu<< kVerDisp);
    }
    count_ = count;
    next_.store(0);
  }

  template<class T>
  inline void ObjectPool<T>::reset() {
    count_ = 0;
    next_.store(0);
    if (data_) {
      mem_.free_fn(data_);
      data_ = nullptr;
    }
  }

  // only access objects you've previously referenced
  template<class T>
  inline T& ObjectPool<T>::get(uint32_t hnd) {
    uint32_t pos = hnd & kPosMask;
    PX_SCHED_CHECK_FN(pos < count_, "Invalid access to pos %u hnd:%zu", pos, count_);
    return data_[pos].element;
  }

  // only access objects you've previously referenced
  template< class T>
  inline const T&  ObjectPool<T>::get(uint32_t hnd) const {
    uint32_t pos = hnd & kPosMask;
    PX_SCHED_CHECK_FN(pos < count_, "Invalid access to pos %zu hnd:%zu", pos, count_);
    return data_[pos].element;
  }

  template< class T>
  inline uint32_t ObjectPool<T>::info(size_t pos, uint32_t *count, uint32_t *ver) const {
    PX_SCHED_CHECK_FN(pos < count_, "Invalid access to pos %zu hnd:%zu", pos, count_);
    uint32_t s = data_[pos].state.load();
    if (count) *count = (s & kRefMask);
    if (ver) *ver = (s & kVerMask) >> kVerDisp;
    return (s&kVerMask) | pos;
  }

  template<class T>
  inline uint32_t ObjectPool<T>::adquireAndRef() {
    uint32_t tries = 0;
    for(;;) {
      uint32_t pos = (next_.fetch_add(1)%count_);
      D& d = data_[pos];
      uint32_t version = (d.state.load() & kVerMask) >> kVerDisp;
      // note: avoid 0 as version
      uint32_t newver = (version+1) & 0xFFF;
      if (newver == 0) newver = 1;
      // instead of using 1 as initial ref, we use 2, when we see 1
      // in the future we know the object must be freed, but it wont
      // be actually freed until it reaches 0
      uint32_t newvalue = (newver << kVerDisp) + 2;
      uint32_t expected = version << kVerDisp;
      if (d.state.compare_exchange_strong(expected, newvalue)) {
        newElement(pos); //< initialize
        return (newver << kVerDisp) | (pos & kPosMask);
      }
      tries++;
      // TODO... make this, optional...
      PX_SCHED_CHECK_FN(tries < count_*count_, "It was not possible to find a valid index after %u tries", tries);
    }
  }

  template< class T>
  inline void ObjectPool<T>::unref(uint32_t hnd) const {
    uint32_t pos = hnd & kPosMask;
    uint32_t ver = (hnd & kVerMask);
    D& d = data_[pos];
    for(;;) {
      uint32_t prev = d.state.load();
      uint32_t next = prev - 1;
      PX_SCHED_CHECK_FN((prev & kVerMask) == ver,
          "Invalid unref HND = %u(%u), Versions: %u vs %u",
          pos, hnd, prev & kVerMask, ver);
      PX_SCHED_CHECK_FN((prev & kRefMask) > 1,
          "Invalid unref HND = %u(%u), invalid ref count",
          pos, hnd);
      if (d.state.compare_exchange_strong(prev, next)) {
        if ((next & kRefMask) == 1) {
          deleteElement(pos);
          d.state.store(0);
        }
        return;
      }
    }
  }

  // decrements the counter, if the objet is no longer valid (las ref)
  // the given function will be executed with the element
  template<class T>
  template<class F>
  inline void ObjectPool<T>::unref(uint32_t hnd, F f) const {
    uint32_t pos = hnd & kPosMask;
    uint32_t ver = (hnd & kVerMask);
    D& d = data_[pos];
    for(;;) {
      uint32_t prev = d.state.load();
      uint32_t next = prev - 1;
      PX_SCHED_CHECK_FN((prev & kVerMask) == ver,
          "Invalid unref HND = %u(%u), Versions: %u vs %u",
          pos, hnd, prev & kVerMask, ver);
      PX_SCHED_CHECK_FN((prev & kRefMask) > 1,
          "Invalid unref HND = %u(%u), invalid ref count",
          pos, hnd);
      if (d.state.compare_exchange_strong(prev, next)) {
        if ((next & kRefMask) == 1) {
          f(d.element);
          deleteElement(pos);
          d.state.store(0);
        }
        return;
      }
    }
  }

  template< class T>
  inline bool ObjectPool<T>::ref(uint32_t hnd) const{
    if (!hnd) return false;
    uint32_t pos = hnd & kPosMask;
    uint32_t ver = (hnd & kVerMask);
    D& d = data_[pos];
    for (;;) {
      uint32_t prev = d.state.load();
      uint32_t next_c =((prev & kRefMask) +1);
      if ((prev & kVerMask) != ver || next_c <= 2) return false;
      PX_SCHED_CHECK_FN(next_c  == (next_c & kRefMask), "Too many references...");
      uint32_t next = (prev & kVerMask) | next_c ;
      if (d.state.compare_exchange_strong(prev, next)) {
        return true;
      }
    }
  }

  template< class T>
  inline uint32_t ObjectPool<T>::refCount(uint32_t hnd) const{
    if (!hnd) return 0;
    uint32_t pos = hnd & kPosMask;
    uint32_t ver = (hnd & kVerMask);
    D& d = data_[pos];
    uint32_t current = d.state.load();
    if ((current & kVerMask) != ver ) return 0;
    return (current & kRefMask);
  }


} // end of px namespace
#endif // PX_SCHED

//----------------------------------------------------------------------------
// -- IMPLEMENTATION ---------------------------------------------------------
//----------------------------------------------------------------------------

#ifdef PX_SCHED_IMPLEMENTATION

#ifndef PX_SCHED_IMPLEMENTATION_DONE
#define PX_SCHED_IMPLEMENTATION_DONE 1

#ifdef PX_SCHED_ATLERNATIVE_TLS
// used to store custom TLS... some platform might have problems with
// TLS (iOS used to be one)
#include <unordered_map>
#endif

#if PX_SCHED_CHECK_DEADLOCKS
#include <vector>
#include <algorithm>
#endif


namespace px_sched {

  struct Scheduler::TLS {
    const char *name = nullptr;
    Scheduler *scheduler = nullptr;
    struct Resource {
      const void *ptr;
      const char *name;
    };
    Resource next_lock = {nullptr, nullptr};
#if PX_SCHED_CHECK_DEADLOCKS
    std::mutex adquired_locks_m;
    std::vector<Resource> adquired_locks;
#endif
  };

  Scheduler::TLS* Scheduler::tls() {
#ifdef PX_SCHED_ATLERNATIVE_TLS
    static std::unordered_map<std::thread::id, TLS> data;
    static Atomic<uint32_t> in_use = {0};
    for(;;) {
      uint32_t expected = 0;
      if (in_use.compare_exchange_weak(expected, 1)) break;
    }
    auto result = &data[std::this_thread::get_id()];
    in_use = false;
    return result;
#else
    static thread_local TLS tls;
    return &tls;
#endif
  }

  void Scheduler::set_current_thread_name(const char *name) {
    TLS *d = tls();
    d->name = name;
  }

  const char *Scheduler::current_thread_name() {
    TLS *d = tls();
    return d->name;
  }

  void Scheduler::CurrentThreadSleeps() {
    CurrentThreadBeforeLockResource(nullptr);
  }

  void Scheduler::CurrentThreadWakesUp() {
    CurrentThreadAfterLockResource(false);
  }

  void Scheduler::CurrentThreadBeforeLockResource(const void *resource_ptr, const char *name) {
    // if the lock might work, wake up one thread to replace this one
    TLS *d = tls();
    if (d->scheduler && d->scheduler->running_.load()) {
      d->scheduler->active_threads_.fetch_sub(1);
      d->scheduler->wakeUpOneThread();
    }
    d->next_lock = {resource_ptr, name};
  }

  void Scheduler::CurrentThreadAfterLockResource(bool success) {
    // mark this thread as active (so eventually one thread will step down)
    TLS *d = tls();
    if (d->scheduler && d->scheduler->running_.load()) {
      d->scheduler->active_threads_.fetch_add(1);
    }
    if (success && d->next_lock.ptr) {
#if PX_SCHED_CHECK_DEADLOCKS
      std::lock_guard<std::mutex> l(d->adquired_locks_m);
      d->adquired_locks.push_back(d->next_lock);
#endif
    }
    d->next_lock = {nullptr,nullptr}; // reset
  }

  void Scheduler::CurrentThreadReleasesResource(const void *resource_ptr) {
#if PX_SCHED_CHECK_DEADLOCKS
    TLS *d = tls();
    if (resource_ptr) {
      std::lock_guard<std::mutex> l(d->adquired_locks_m);
      auto f = d->adquired_locks.begin();
      while (f != d->adquired_locks.end()) {
        if (f->ptr == resource_ptr) break;
        f++;
      };
      PX_SCHED_CHECK_FN(f != d->adquired_locks.end(), "Can't find resource %p as adquired", resource_ptr);
      // replace resource with last, and pop (to avoid shifting elements in the vector)
      std::swap(*f, d->adquired_locks.back());
      d->adquired_locks.pop_back();
    }
#else
    (void)resource_ptr;
#endif
  }
}

// Common to all implementations of px_sched (Single Threaded and Multi Threaded)
namespace px_sched {
  uint32_t Scheduler::createCounter() {
    uint32_t hnd = counters_.adquireAndRef();
    Counter *c = &counters_.get(hnd);
    c->task_id.store(0);
    c->user_count.store(0);
    c->wait_ptr = nullptr;
    return hnd;
  }

  uint32_t Scheduler::createTask(const Job &job, Sync *sync_obj) {
    uint32_t ref = tasks_.adquireAndRef();
    Task *task = &tasks_.get(ref);
    task->job = job;
    task->counter_id = 0;
    task->next_sibling_task.store(0);
    if (sync_obj) {
      bool new_counter = !counters_.ref(sync_obj->hnd);
      if (new_counter) {
        sync_obj->hnd = createCounter();
      }
      task->counter_id = sync_obj->hnd;
    }
    return ref;
  }

  void Scheduler::incrementSync(Sync *s) {
    bool new_counter = false;
    if (!counters_.ref(s->hnd)) {
      s->hnd = createCounter();
      new_counter = true;
    }
    Counter &c = counters_.get(s->hnd);
    c.user_count.fetch_add(1);
    if (!new_counter) {
      unrefCounter(s->hnd);
    }
  }

  void Scheduler::decrementSync(Sync *s) {
    if (counters_.ref(s->hnd)) {
      Counter &c = counters_.get(s->hnd);
      uint32_t prev = c.user_count.fetch_sub(1);
      if (prev == 1) {
        // last one should unref twice
        unrefCounter(s->hnd);
      }
      unrefCounter(s->hnd);
    }
  }
}

#if PX_SCHED_IMP_SINGLE_THREAD

namespace px_sched {
  Scheduler::Scheduler() {}
  Scheduler::~Scheduler() {}
  void Scheduler::init(const SchedulerParams &) {
    tasks_.init(params_.max_number_tasks, params_.mem_callbacks);
    counters_.init(params_.max_number_tasks, params_.mem_callbacks);
  }
  void Scheduler::stop() {
    tasks_.reset();
    counters_.reset();
  }
  void Scheduler::run(const Job &job, Sync *s) {
    Job j(job);
    j();
    if (s) decrementSync(s);
  }

  void Scheduler::runAfter(Sync trigger, const Job &job, Sync *s) {
    if (counters_.ref(trigger.hnd)) {
      uint32_t t_ref = createTask(job, s);
      Counter *c = &counters_.get(trigger.hnd);
      for(;;) {
        uint32_t current = c->task_id.load();
        if (c->task_id.compare_exchange_strong(current, t_ref)) {
          Task *task = &tasks_.get(t_ref);
          task->next_sibling_task.store(current);
          break;
        }
      }
      unrefCounter(trigger.hnd);
    } else {
      run(job, s);
    }
  }

  void Scheduler::waitFor(Sync s) {
    PX_SCHED_CHECK_FN(!(counters_.ref(s.hnd)), "Invalid, on SingleThreaded mode we can not wait for a sync object...");
  }

  uint32_t Scheduler::numPendingTasks(Sync s){
    return counters_.refCount(s.hnd);
  }

  void Scheduler::getDebugStatus(char *buffer, size_t buffer_size) {
    if (buffer_size) buffer[0] = 0;
  }

  void Scheduler::unrefCounter(uint32_t hnd) {
    if (counters_.ref(hnd)) {
      counters_.unref(hnd);
      Scheduler *schd = this;
      counters_.unref(hnd, [schd](Counter &c) {
        // wake up all tasks 
        uint32_t tid = c.task_id.load();
        while (schd->tasks_.ref(tid)) {
          Task &task = schd->tasks_.get(tid);
          uint32_t next_tid = task.next_sibling_task.load(); 
          uint32_t counter_id = task.counter_id;
          task.next_sibling_task.store(0);
          task.job(); // execute the task
          schd->tasks_.unref(tid);
          schd->unrefCounter(counter_id);
          tid = next_tid;
        }
      });
    }
  }

  void Scheduler::wakeUpOneThread() {}
} // end of px namespace
#endif // PX_SCHED_IMP_SINGLE_THREAD

#if PX_SCHED_IMP_REGULAR_THREADS
// Default implementation using threads 
#include <thread>
namespace px_sched {
  Scheduler::Scheduler() {
    active_threads_.store(0);
  }

  Scheduler::~Scheduler() { stop(); }

  void Scheduler::init(const SchedulerParams &_params) {
    stop();
    running_.store(true);
    params_ = _params;
    if (params_.max_running_threads == 0) {
      params_.max_running_threads = static_cast<uint16_t>(std::thread::hardware_concurrency());
    }
    // create tasks
    tasks_.init(params_.max_number_tasks, params_.mem_callbacks);
    counters_.init(params_.max_number_tasks, params_.mem_callbacks);
    ready_tasks_.init(params_.max_number_tasks, params_.mem_callbacks);
    PX_SCHED_CHECK_FN(workers_ == nullptr, "workers_ ptr should be null here...");
    workers_ = static_cast<Worker*>(params_.mem_callbacks.alloc_fn(sizeof(Worker)*params_.num_threads));
    for(uint16_t i = 0; i < params_.num_threads; ++i) {
      new (&workers_[i]) Worker();
      workers_[i].thread_index = i;
    }
    PX_SCHED_CHECK_FN(active_threads_.load() == 0, "Invalid active threads num");
    for(uint16_t i = 0; i < params_.num_threads; ++i) {
      workers_[i].thread = std::thread(WorkerThreadMain, this, &workers_[i]);
    }
  }

  void Scheduler::stop() {
    if (running_.load()) {
      running_.store(false);
      for(uint16_t i = 0; i < params_.num_threads; ++i) {
        wakeUpThreads(params_.num_threads);
      }
      for(uint16_t i = 0; i < params_.num_threads; ++i) {
        workers_[i].thread.join();
        workers_[i].~Worker();
      }
      params_.mem_callbacks.free_fn(workers_);
      workers_ = nullptr;
      tasks_.reset();
      counters_.reset();
      ready_tasks_.reset();
      PX_SCHED_CHECK_FN(active_threads_.load() == 0, "Invalid active threads num --> %u", active_threads_.load());
    }
  }
  
  void Scheduler::getDebugStatus(char *buffer, size_t buffer_size) {
    size_t p = 0;
    int n = 0;
    #define _ADD(...) {p += static_cast<size_t>(n); (p < buffer_size) && (n = snprintf(buffer+p, buffer_size-p,__VA_ARGS__));}
    _ADD("Workers:0    5    10   15   20   25   30   35   40   45   50   55   60   65   70   75\n");
    _ADD("%3u/%3u:", active_threads_.load(), params_.max_running_threads);
    for(size_t i = 0; i < params_.num_threads; ++i) {
      _ADD( (workers_[i].wake_up.load() == nullptr)?"*":".");
    }
    _ADD("\nWorkers(%d):", params_.num_threads);
    for(size_t i = 0; i < params_.num_threads; ++i) {
      auto &w = workers_[i];
      bool is_on =(w.wake_up.load() == nullptr);
      bool has_something_to_show = w.thread_tls->next_lock.ptr;
#if PX_SCHED_CHECK_DEADLOCKS
      std::lock_guard<std::mutex> l(w.thread_tls->adquired_locks_m);
      has_something_to_show = has_something_to_show || w.thread_tls->adquired_locks.size();
#endif
      if (!is_on && !has_something_to_show) {
        continue;
      }
      _ADD("\n  Worker: %d(%s) %s", w.thread_index, 
          is_on?"ON":"OFF",
          w.thread_tls->name? w.thread_tls->name: "-no-name-"
          );
#if PX_SCHED_CHECK_DEADLOCKS
      if (w.thread_tls->adquired_locks.size()) {
        _ADD("\n    AdquiredLocks:");
        for(auto ptr:w.thread_tls->adquired_locks) {
          if (ptr.name) {
            _ADD("%p(%s) ",ptr.ptr, ptr.name);
          } else {
            _ADD("%p ",ptr.ptr);
          }
        }
      }
#endif
      if (w.thread_tls->next_lock.ptr) {
        if (w.thread_tls->next_lock.name) {
          _ADD("\n    Waiting For Lock: %p(%s)", w.thread_tls->next_lock.ptr, w.thread_tls->next_lock.name);
        } else {
          _ADD("\n    Waiting For Lock: %p", w.thread_tls->next_lock.ptr);
        }
      }
    }
    _ADD("\nReady: ");
    for(size_t i = 0; i < ready_tasks_.in_use_; ++i) {
      _ADD("%d,",ready_tasks_.list_[i]);
    }
    _ADD("\nTasks: ");
    for(size_t i = 0; i < tasks_.size(); ++i) {
      uint32_t c,v;
      uint32_t hnd = tasks_.info(i, &c, &v);
      if (c>0) { _ADD("%u,",hnd); }
    }
    _ADD("\nCounters:");
    for(size_t i = 0; i < counters_.size(); ++i) {
      uint32_t c,v;
      uint32_t hnd = counters_.info(i, &c, &v);
      if (c>0) { _ADD("%u,",hnd); }
    }
    _ADD("\n");
    #undef _ADD
  }

  uint16_t Scheduler::wakeUpThreads(uint16_t max_num_threads) {
    uint16_t total_woken_up = 0;
    for(uint32_t i = 0; (i < params_.num_threads) && (total_woken_up < max_num_threads); ++i) {
      WaitFor *wake_up = workers_[i].wake_up.exchange(nullptr);
      if (wake_up) {
        wake_up->signal();
        total_woken_up++;
        // Add one to the total active threads, for later substracting it, this
        // will take the thread as awake before the thread actually is again working
        active_threads_.fetch_add(1);
      }
    }
    active_threads_.fetch_sub(total_woken_up);
    return total_woken_up;
  }

  void Scheduler::wakeUpOneThread() {
    for(;;) {
      uint32_t active = active_threads_.load();
      if ((active >= params_.max_running_threads) ||
          wakeUpThreads(1)) return;
    }
  }

  void Scheduler::run(const Job &job, Sync *sync_obj) {
    PX_SCHED_CHECK_FN(running_.load(), "Scheduler not running");
    uint32_t t_ref = createTask(job, sync_obj);
    ready_tasks_.push(t_ref);
    wakeUpOneThread();
  }

  void Scheduler::runAfter(Sync _trigger, const Job& _job, Sync* _sync_obj) {
    PX_SCHED_CHECK_FN(running_.load(), "Scheduler not running");
    uint32_t trigger = _trigger.hnd;
    uint32_t t_ref = createTask(_job, _sync_obj);
    bool valid = counters_.ref(trigger);
    if (valid) {
      Counter *c = &counters_.get(trigger);
      for(;;) {
        uint32_t current = c->task_id.load();
        if (c->task_id.compare_exchange_strong(current, t_ref)) {
          Task *task = &tasks_.get(t_ref);
          task->next_sibling_task.store(current);
          break;
        }
      }
      unrefCounter(trigger);
    } else {
      ready_tasks_.push(t_ref);
      wakeUpOneThread();
    }
  }

  void Scheduler::waitFor(Sync s) {
    if (counters_.ref(s.hnd)) {
      Counter &counter = counters_.get(s.hnd);
      PX_SCHED_CHECK_FN(counter.wait_ptr == nullptr, "Sync object already used for waitFor operation, only one is permited");
      WaitFor wf;
      counter.wait_ptr = &wf;
      unrefCounter(s.hnd);
      CurrentThreadSleeps(); 
      wf.wait();
      CurrentThreadWakesUp(); 
    }
  }

  uint32_t Scheduler::numPendingTasks(Sync s) {
    return counters_.refCount(s.hnd);
  }

  void Scheduler::unrefCounter(uint32_t hnd) {
    if (counters_.ref(hnd)) {
      counters_.unref(hnd);
      Scheduler *schd = this;
      counters_.unref(hnd, [schd](Counter &c) {
        // wake up all tasks 
        uint32_t tid = c.task_id.load();
        while (schd->tasks_.ref(tid)) {
          Task &task = schd->tasks_.get(tid);
          uint32_t next_tid = task.next_sibling_task.load(); 
          task.next_sibling_task.store(0);
          schd->ready_tasks_.push(tid);
          schd->wakeUpOneThread();
          schd->tasks_.unref(tid);
          tid = next_tid;
        }
        if (c.wait_ptr) {
          c.wait_ptr->signal();
        }
      });
    }
  }

  void Scheduler::WorkerThreadMain(Scheduler *schd, Scheduler::Worker *worker_data) {
    char buffer[16];

    const uint16_t id = worker_data->thread_index;
    TLS *local_storage = tls();

    local_storage->scheduler = schd;
    worker_data->thread_tls = local_storage;

    auto const ttl_wait = schd->params_.thread_sleep_on_idle_in_microseconds;
    auto const ttl_value = schd->params_.thread_num_tries_on_idle? schd->params_.thread_num_tries_on_idle:1;
    schd->active_threads_.fetch_add(1);
    snprintf(buffer,16,"Worker-%u", id);
    schd->set_current_thread_name(buffer);
    for(;;) {
      { // wait for new activity
        auto current_num = schd->active_threads_.fetch_sub(1);
        if (!schd->running_.load()) return;
        if (schd->ready_tasks_.in_use() == 0 ||
            current_num > schd->params_.max_running_threads) {
          WaitFor wf;
          schd->workers_[id].wake_up.store(&wf);
          wf.wait();
          if (!schd->running_.load()) return;
        }
        schd->active_threads_.fetch_add(1);
        schd->workers_[id].wake_up.store(nullptr);
      }
      auto ttl = ttl_value;
      { // do some work
        uint32_t task_ref;
        while (ttl && schd->running_.load()) {
          if (!schd->ready_tasks_.pop(&task_ref)) {
            ttl--;
            if (ttl_wait) std::this_thread::sleep_for(std::chrono::microseconds(ttl_wait));
            continue;
          }
          ttl = ttl_value;
          Task *t = &schd->tasks_.get(task_ref);
          t->job();
          uint32_t counter = t->counter_id;
          schd->tasks_.unref(task_ref);
          schd->unrefCounter(counter);
        }
      }
    }
    worker_data->thread_tls = nullptr;
    local_storage->scheduler = nullptr;
    schd->set_current_thread_name(nullptr);
  }
} // end of px_sched namespace
#endif // PX_SCHED_IMP_REGULAR_THREADS

#endif // PX_SCHED_IMPLEMENTATION_DONE

#endif // PX_SCHED_IMPLEMENTATION
