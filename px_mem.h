/* -----------------------------------------------------------------------------
Copyright (c) 2018 Jose L. Hidalgo (PpluX)

  px_mem.h - Mem management functions
  Single header file to handle Mem references, Buffers, unique_ptr/array
  alike objects and so on. It offers safer objects to handle pointers to
  memory and a RAII implementation for dynamic allocated memory.
     
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
//
// In *ONE* C++ file you need to declare
// #define PX_MEM_IMPLEMENTATION
// before including the file that contains px_mem.h


#ifndef PX_MEM
#define PX_MEM 1

#include <cstddef>
#include <cstdint>
#include <memory> //> for std::align

#ifndef PX_MEM_ASSERT
#include <cassert>
#define PX_MEM_ASSERT(cnd, msg) assert((cnd) && msg)
#endif

namespace px {

  // Set this first, before anything else, the default implementation will
  // use a wrapper around malloc and free
  void SetMemoryFunctions(
    void*(*MemAlloc)(size_t size_bytes, size_t alignment),
    void (*MemFree)(void*));

  // Function to adquire memory (with proper alignment)
  void *MemoryAlloc(size_t amount, size_t alignment);

  // Function to release a previously adquired memory
  void MemoryFree(void *ptr);

  // Class to handle references to array of objects in memory
  template<class T>
  class ConstMemRef {
  public:
    ConstMemRef() : ptr_(nullptr), count_(0) {}
    ConstMemRef(const T* ptr, size_t num) : ptr_(ptr), count_(num) {}
    ConstMemRef(const ConstMemRef &)  = default;
    ConstMemRef& operator=(const ConstMemRef &) = default;
    constexpr ConstMemRef(ConstMemRef &&)  = default;
    ConstMemRef& operator=(ConstMemRef &&) = default;

    const T& operator[](size_t p) const { PX_MEM_ASSERT(p < count_, "Invalid access");return ptr_[p]; }
    size_t size() const { return count_; }
    size_t sizeInBytes() const { return count_*sizeof(T); }
    const T* get() const { return ptr_; }
  private:
    const T *ptr_;
    size_t count_;
  };

  // Mem<T> similar to unique_ptr but with some restrictions to fully implement RAII and
  // avoid any new/delete
  template<class T>
  class Mem {
  public:
    Mem() {}
    ~Mem() { reset(); }

    Mem(Mem &&other) {
      ptr_ = other.ptr_;
      other.ptr_ = nullptr;
    }

    Mem& operator=(Mem &&other) {
      reset();
      ptr_ = other.ptr_;
      other.ptr_ = nullptr;
      return *this;
    }

    bool valid() const { return ptr_ != nullptr; }
    operator bool() const { return valid(); }

    // instance an object of type T
    T* alloc() { reset(); ptr_ = new (MemoryAlloc(sizeof(T), alignof(T))) T(); return ptr_; }

    // alloc for derived classes
    template<class D>
    D* alloc() {
      reset();
      D *result = new (MemoryAlloc(sizeof(D), alignof(T))) D();
      ptr_ = result;
      return result;	
    }

    void reset() {
      if (ptr_) {
        ptr_->~T();
        MemoryFree(ptr_);
        ptr_ = nullptr;
      }
    }

    T* get() { return ptr_; }
    const T* get() const { return ptr_; }

    T* operator->() { return ptr_; }
    const T* operator->() const { return ptr_; }
    T& operator*() { return *ptr_; }
    const T& operator*() const { return *ptr_; }

  private:
    T *ptr_ = nullptr;
  };



  template<class T>
  class Mem<T[]> {
  public:
    typedef T* iterator;
    typedef const T* const_iterator;

    Mem() {}
    ~Mem() { reset(); }

    Mem(Mem &&other) {
      ptr_ = other.ptr_;
      size_ = other.size_;
      other.ptr_ = nullptr;
      other.size_ = 0;
    }

    Mem& operator=(Mem &&other) {
      reset();
      ptr_ = other.ptr_;
      size_ = other.size_;
      other.ptr_ = nullptr;
      other.size_ = 0;
      return *this;
    }

    bool valid() const { return ptr_ != nullptr; }
    operator bool() const { return valid(); }

    T* alloc(size_t num) {
      reset();
      ptr_ = reinterpret_cast<T*>(MemoryAlloc(sizeof(T)*num, alignof(T)));
      size_ = num;
      for(size_t i = 0; i < size_; ++i) {
        new (&ptr_[i]) T();
      }
      return ptr_;
    }

    void reset() {
      if (ptr_) {
        for(size_t i = 0; i < size_; ++i) {
          ptr_[i].~T();
        }
        MemoryFree(ptr_);
        ptr_ = nullptr;
        size_ = 0;
      }
    }

    void copy(const T* begin, const T* end) {
      T *dst = alloc(((size_t)end-(size_t)begin)/sizeof(T));
      for(const T *p = begin; p != end; ++p, ++dst) {
        *dst = *p;
      }
    }

    void copy(ConstMemRef<T> memref) {
      if (memref.size() == 0) {
        reset();
      } else { 
        T *dst = alloc(memref.size());
        for(size_t i = 0; i < memref.size(); ++i) {
          dst[i] = memref[i];
        }
      }
    }

    T& operator[](size_t i) {
      PX_MEM_ASSERT(i < size_, "Invalid Access");
      return ptr_[i];
    }

    const T& operator[](size_t i) const {
      PX_MEM_ASSERT(i < size_, "Invalid Access");
      return ptr_[i];
    }

    iterator begin() const {
      return ptr_;
    }
    iterator end() const {
      return ptr_+size_;
    }
    const_iterator cbegin() const {
      return ptr_;
    }
    const_iterator cend() const {
      return ptr_+size_;
    }

    size_t size() const { return size_; }
    size_t sizeInBytes() const { return size_*sizeof(T); }

    T* get() { return ptr_; }
    const T* get() const { return ptr_; }

    operator ConstMemRef<T>() const { return ref(); }
    ConstMemRef<T> ref() const { return ConstMemRef<T>(ptr_, size()); }
    
  private:
    T *ptr_ = nullptr;
    size_t size_ = 0;
  };

  // minimal standard allocator that uses MemoryAlloc/MemoryFree functions
  template <class T>
  struct Allocator {
    typedef T value_type;
    Allocator() {}
    template <class U> Allocator(const Allocator<U>& other) {}
    T* allocate(std::size_t n) { 
      return (T*)MemoryAlloc(sizeof(T)*n, alignof(T));
    }
    void deallocate(T* p, std::size_t n) {
      MemoryFree(p);
    }
  };
  template <class T, class U>
    bool operator==(const Allocator<T>&, const Allocator<U>&) { return false; }
  template <class T, class U>
    bool operator!=(const Allocator<T>&, const Allocator<U>&) { return true; } 

#ifdef PX_MEM_IMPLEMENTATION
  namespace {
    void* _DefaultMemoryAlloc(size_t mem_size, size_t align) {
      size_t mem_plus_align = mem_size+align;
      void *raw_mem = std::malloc(mem_plus_align+sizeof(void*));
      void *ptr = ((char*)raw_mem)+sizeof(void*);
      void *result = std::align(align, mem_size, ptr , mem_plus_align);
      PX_MEM_ASSERT(result != nullptr, "Default Memory Alloc failed");
      ((void**)result)[-1] = raw_mem;
      return result;
    }

    void _DefaultMemoryFree(void *ptr) {
      void *raw_mem = ((void**)ptr)[-1];
      std::free(raw_mem);
    }
  }

  static struct {
    void *(*alloc)(size_t, size_t ) = _DefaultMemoryAlloc;
    void (*free)(void*) = _DefaultMemoryFree;
  } GLOBAL_mem;
  
  void *MemoryAlloc(size_t amount, size_t alignment) {
    return GLOBAL_mem.alloc(amount, alignment);
  }

  void MemoryFree(void *ptr) {
    return GLOBAL_mem.free(ptr);
  }

  void SetMemoryFunctions( void*(*MemAlloc)(size_t, size_t), void (*MemFree)(void*)) {
    if (!MemAlloc && !MemFree) {
      MemAlloc = _DefaultMemoryAlloc;
      MemFree = _DefaultMemoryFree;
    }
    GLOBAL_mem.alloc = MemAlloc;
    GLOBAL_mem.free = MemFree;
  }
#endif

} // px

#endif
