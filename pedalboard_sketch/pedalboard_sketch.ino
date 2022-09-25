
#include <Arduino.h>
#include <EEPROM.h>

#include "array.hpp"
#include <Array.h>

namespace harddefs {
  /** number of inputs on the pedalboard */
  static constexpr const uint8_t channels_count = 8;

  /** map between software indices and hardware wiring */
  static constexpr const unsigned int leds_pins[channels_count]     = {6, 7, 0, 1, 5, 4, 3, 2};
  static constexpr const unsigned int switches_pins[channels_count] = {A3, A4, 10, 11, A2, A1, A0, 12};

  /** returns hardware pin of given io */
  constexpr int led_pin(unsigned int i)    { return 2 + leds_pins[i]; }
  constexpr int switch_pin(unsigned int i) { return switches_pins[channels_count - i - 1]; }

  /** number of expression pedals */
  static constexpr const uint8_t exprs_count = 2;

  /** map between soft and hardware */
  static constexpr const unsigned int exprs_pins[exprs_count] = {A6, A7};

  /** return hardware pin of given expression pedal */
  constexpr int expr_pin(unsigned int i) { return exprs_pins[i]; }

  /** Other constants **/
  static constexpr const size_t serial_buffer_size  = 64;
  
  static constexpr const size_t inputs_count        = channels_count + exprs_count;
  static constexpr const size_t outputs_count       = channels_count + exprs_count;
}

namespace datastore {

  /** Struct definitions **/

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
  
  struct footswitch {
    enum state : uint8_t {
      Released = 0,
      Pressed = 1
    };
    int16_t id;
    state   s;
  };
  
  struct led {
    enum state : uint8_t {
      Off = 0,
      On = 1,
      Blink = 2
    };
    int16_t id;
    state   s;
  };
  
  struct expr {
    enum state : uint8_t {
      Disabled = 0,
      Enabled = 1
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

  struct global {
    hw::array<entry<footswitch>, harddefs::channels_count>  switches;
    hw::array<entry<led>,        harddefs::channels_count>  leds;
    hw::array<entry<expr>,       harddefs::exprs_count>     exprs;
    hw::array<unsigned long,     harddefs::channels_count>  debounce_sw_timers;

    const config* cfg;

    void init(const config* cfg) {
      this->cfg = cfg;
      /* retrieve initial values */
      switches.fill([](int16_t i){
          int pin = harddefs::switch_pin(i);
          pinMode(pin, INPUT_PULLUP);
          return footswitch{i, static_cast<footswitch::state>(digitalRead(pin))};
        });
      exprs.fill([](int16_t i) {
          return expr{i, expr::state::Disabled,
            static_cast<uint16_t>(analogRead(harddefs::expr_pin(i)))};
        });
      leds.fill([](int16_t i){
          pinMode(harddefs::led_pin(i), OUTPUT);
          return led{i, led::state::Off};
        });
      debounce_sw_timers.fill([](int16_t){ return millis(); });
      /* update */
      push_changes();
    }

    void begin_frame() {
      for (auto& sw : switches) sw.reset_changed();
      for (auto& led: leds)     led.reset_changed();
      for (auto& ex : exprs)    ex.reset_changed();
    }

    void read_inputs() {
      /** Switches **/
      unsigned long t = millis();
      for (auto& sw : switches) {
        /** Pass during debouncing **/
        if ((t - debounce_sw_timers[sw.get().id]) < cfg->debounce_sw_duration)
          continue;
          
        auto v = digitalRead(harddefs::switch_pin(sw.get().id));
        auto s = v == HIGH ? footswitch::state::Pressed : footswitch::state::Released;
        if (s != sw.get().s) {
          sw.set().s = s;
          debounce_sw_timers[sw.get().id] = t;
        }
      }
      /** Expressions **/
      for (auto& ex : exprs) {
        if (ex.get().s == expr::state::Disabled)
          continue;
        auto v = static_cast<uint16_t>(analogRead(harddefs::expr_pin(ex.get().id)));
        if (v != ex.get().value)
          ex.set().value = v;
      }
    }

    void push_changes() const {
      for (const auto& sw : switches) {
        if (!sw.changed())
          continue;
        Serial.print("SW changed : "); Serial.print(sw.get().id);
        Serial.print(" : "); Serial.println(static_cast<int>(sw.get().s));
        digitalWrite(harddefs::led_pin(sw.get().id), static_cast<int>(sw.get().s));
      }
//      for (const auto& l : leds) {
//        if (!l.changed())
//          continue;
//         int state = l.get().s == led::state::On ? HIGH : LOW;
//         digitalWrite(harddefs::led_pin(l.get().id), state);
//      }
      for (const auto& ex : exprs) {
        if (!ex.changed())
          continue;
        Serial.print("Expr changed : "); Serial.print(ex.get().id);
        Serial.print(" : "); Serial.println(static_cast<int>(ex.get().value));
      }
    }

    void dump() const {
      for (const auto& sw : switches) {
        Serial.print("SW : "); Serial.print(sw.get().id);
        Serial.print(" : "); Serial.print(harddefs::switch_pin(sw.get().id));
        Serial.print(" : "); Serial.print(debounce_sw_timers[sw.get().id]);
        Serial.print(" : "); Serial.println(static_cast<int>(sw.get().s));
      }
      for (const auto& led : leds) {
        Serial.print("Led : "); Serial.print(led.get().id);
        Serial.print(" : "); Serial.println(static_cast<int>(led.get().s));
      }
      for (const auto& ex : exprs) {
        Serial.print("Expr : "); Serial.print(ex.get().id);
        Serial.print(" : "); Serial.println(static_cast<int>(ex.get().value));
      }
    }
  };

  /** Global objects **/
  
  static config configs;
  static global globals;
}

void setup() {
  
  /** Hardware setup **/
  
  Serial.begin(9600);
  
  datastore::globals.init(&datastore::configs);
  datastore::globals.dump();

  /** beautiful animation **/

}

void loop() {
//*
  datastore::globals.begin_frame();
  datastore::globals.read_inputs();
  datastore::globals.push_changes();
//*/

/*
  for (size_t i = 0; i < harddefs::channels_count; ++i) {
    digitalWrite(harddefs::led_pin(i), digitalRead(harddefs::switch_pin(i)));
  }
//*/

/*
  for (size_t i = 0; i < harddefs::channels_count; ++i) {
    digitalWrite(harddefs::led_pin(i), HIGH);
    delay(50);
  }
  for (size_t i = 0; i < harddefs::channels_count; ++i) {
    digitalWrite(harddefs::led_pin(i), LOW);
    delay(50);
  }
  for (size_t i = 0; i < harddefs::channels_count; ++i) {
    digitalWrite(harddefs::led_pin(i), HIGH);
    delay(50);
    digitalWrite(harddefs::led_pin(i), LOW);
  }
//*/
}
