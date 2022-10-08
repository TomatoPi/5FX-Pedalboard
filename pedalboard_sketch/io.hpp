#pragma once
#ifndef _IO_HPP_
#define _IO_HPP_

#include <stdint.h>

namespace io
{
  namespace impl
  {
    template <typename T>
    void print(T t) { Serial.print(t); }
    template <typename T, typename... Ts>
    void print(T t, Ts... ts)
    {
      print(t);
      print(ts...);
    }
  }

  template <typename... Ts>
  void print(Ts... ts)
  {
    const uint8_t head[] = {0x70, 0x7D, 0x02};
    Serial.write(head, 3);
    impl::print<Ts...>(ts...);
    Serial.write('\n');
    Serial.write(0xF7);
  }

  template <typename T>
  void write(T t) { Serial.write(t); }
  template <typename T, typename... Ts>
  void write(T t, Ts... ts)
  {
    write(t);
    write(ts...);
  }

  static void present(const char *name)
  {
    const uint8_t head[] = {0x70, 0x7D, 0x01};
    Serial.write(head, 3);
    Serial.write(name);
    Serial.write(0xF7);
  }
}

#endif /* ifndef _IO_HPP_ */