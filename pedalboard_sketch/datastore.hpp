#pragma once

#include "array.hpp"
#include "harddefs.hpp"

#include <stdint.h>

namespace datastore {

  /** Struct definitions **/
  
  struct footswitch {
    enum state : uint8_t {
      Released = 0,
      Pressed = 1,
      Count
    };
    int16_t id;
    state   s;
  };
  
  struct led {
    enum state : uint8_t {
      Off = 0,
      On = 1,
      Blink = 2,
      Count
    };
    int16_t id;
    state   s;
  };
  
  struct expr {
    enum state : uint8_t {
      Disabled = 0,
      Enabled = 1,
      Count
    };
    int16_t   id;
    state     s;
    uint16_t  value;    /**< 10 bits value */
  };

  struct config {
    uint8_t footswitch_cc = 0x04;
    uint8_t expression_cc = 0x0B;
    uint8_t led_cc        = 0x03;

    uint32_t debounce_sw_duration = 50; /**< 50ms */
  };

  template <typename T>
  class entry {
  public :

    entry(const T& val = T()) :
      _value(val), _changed(true)
      {}

    T& set() { _changed = true; return _value; }
    const T& get() const { return _value; }

    bool changed() const { return _changed; }
    void reset_changed() { _changed = false; }

  private:
    T    _value;
    bool _changed; /**< True if state has changed at last frame */
  };

  struct global {
    hw::array<entry<footswitch>, harddefs::channels_count>  switches;
    hw::array<entry<led>,        harddefs::channels_count>  leds;
    hw::array<entry<expr>,       harddefs::exprs_count>     exprs;
    hw::array<unsigned long,     harddefs::channels_count>  debounce_sw_timers;

    const config* cfg;

    void init(const config* cfg);

    void begin_frame();
    void read_inputs();
    void push_changes() const;

    /** debug method used to send whole datastore in one call **/
    void dump() const;
  };
}
