#pragma once

#include <Arduino.h>

namespace hw {
  
  template <typename T, unsigned long Size>
  class array {
  public :
  
    static constexpr unsigned long size = Size;

    const T& operator[] (unsigned long i) const { return storage[i]; }
    T&       operator[] (unsigned long i)       { return storage[i]; }

    const T* begin() const { return storage; }
    T*       begin()       { return storage; }

    const T* end() const { return storage + size; }
    T*       end()       { return storage + size; }

    template <typename Filler>
    void fill(Filler&& fn) {
      for (unsigned long i = 0; i<size; ++i)
        storage[i] = fn(i);
    }

  private :
    T storage[Size];
  };
  
  template <typename T, typename Container, Container* container>
  struct vector {
    
    enum status : uint8_t {
      Success = 0,
      ContainerFull = 1,
      OutOfBoundAccess = 2
    };
    
    static constexpr unsigned long max_size = Container::size;
    
    unsigned long size = 0;
    status last_op     = status::Success;
  
    const T& operator[] (unsigned long i) const { return container[i]; }
    T&       operator[] (unsigned long i)       { return container[i]; }
  
    const T* begin() const { return container->cbegin(); }
    T*       begin()       { return container->begin(); }
  
    const T* end() const { return container->cbegin() + size; }
    T*       end()       { return container->begin() + size; }

    bool is_full()  const { return size == max_size; }
    bool is_empty() const { return size == 0; }
  
    void clear() { size = 0; }
    
    void push_back(const T& val) {
      if (is_full())
        return; /* TODO : set err */
      *container->end() = val;
      size += 1;
    }
  
    void fill(const T& val) {
        container->fill(val);
        size = max_size;
    }
  };
    
}