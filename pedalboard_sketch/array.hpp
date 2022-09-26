#pragma once

#include <Arduino.h>

namespace hw {
  
  template <typename T, unsigned long Size>
  class array {
  public :

    using element_type = T;
  
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
  
  template <typename Container, Container* container>
  struct vector {

    /** Nested types **/
    
    using element_type = typename Container::element_type;
    
    enum status : uint8_t {
      Success = 0,
      ContainerFull = 1,
      OutOfBoundAccess = 2
    };

    /** Members **/
    
    static constexpr unsigned long max_size = Container::size;
    
    unsigned long size = 0;
    status last_op     = status::Success;

    /** Methods **/
  
    const element_type& operator[] (unsigned long i) const { return (*container)[i]; }
    element_type&       operator[] (unsigned long i)       { return (*container)[i]; }
  
    const element_type* begin() const { return container->cbegin(); }
    element_type*       begin()       { return container->begin(); }
  
    const element_type* end() const { return container->cbegin() + size; }
    element_type*       end()       { return container->begin() + size; }

    bool is_full()  const { return size == max_size; }
    bool is_empty() const { return size == 0; }
  
    void clear() { size = 0; }
    
    void push_back(const element_type& val) {
      if (is_full())
        return; /* TODO : set err */
      *end() = val;
      size += 1;
    }
  
    void fill(const element_type& val) {
        container->fill(val);
        size = max_size;
    }
  };
    
}
