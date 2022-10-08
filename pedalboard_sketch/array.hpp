#pragma once

#include <stddef.h>

namespace hw
{

  template <typename T, size_t Size>
  class array
  {
  public:
    using element_type = T;

    static constexpr size_t size = Size;

    const T &operator[](size_t i) const { return storage[i]; }
    T &operator[](size_t i) { return storage[i]; }

    const T *begin() const { return storage; }
    T *begin() { return storage; }

    const T *end() const { return storage + size; }
    T *end() { return storage + size; }

    template <typename Filler>
    void fill(Filler &&fn)
    {
      for (size_t i = 0; i < size; ++i)
        storage[i] = fn(i);
    }

  private:
    T storage[Size];
  };

  template <typename Container, Container *container>
  struct vector
  {

    /** Nested types **/

    using element_type = typename Container::element_type;

    /** Members **/

    static constexpr size_t max_size = Container::size;
    size_t size = 0;

    /** Methods **/

    const element_type &operator[](size_t i) const { return (*container)[i]; }
    element_type &operator[](size_t i) { return (*container)[i]; }

    const element_type *begin() const { return container->cbegin(); }
    element_type *begin() { return container->begin(); }

    const element_type *end() const { return container->cbegin() + size; }
    element_type *end() { return container->begin() + size; }

    bool is_full() const { return size == max_size; }
    bool is_empty() const { return size == 0; }

    void clear() { size = 0; }

    void push_back(const element_type &val)
    {
      if (is_full())
        return; /* TODO : set err */
      *end() = val;
      size += 1;
    }

    void fill(const element_type &val)
    {
      container->fill(val);
      size = max_size;
    }
  };

}
