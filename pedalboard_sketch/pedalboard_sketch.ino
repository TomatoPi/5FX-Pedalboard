
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
  static constexpr const size_t serial_buffer_size  = 128;
  
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
      for (const auto& l : leds) {
        if (!l.changed())
          continue;
         int state = l.get().s == led::state::On ? HIGH : LOW;
         digitalWrite(harddefs::led_pin(l.get().id), state);
      }
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

namespace io {
  
  /** Serial Storage **/
  using serial_storage_type = hw::array<uint8_t, harddefs::serial_buffer_size>;
  static serial_storage_type serial_raw_storage;

  /** Serial buffer **/
  using serial_buffer_type = hw::vector<serial_storage_type, &serial_raw_storage>;
  static serial_buffer_type serial_buffer;

  /** Parser status **/
  enum class status {
    Ok = 0,
    ReceivingCC    = 1,
    ReceivingSysex = 2,
    Rejected       = 3,
  };
  static status serial_status;

  /** Processing methods **/

  void process_cc() {
    uint8_t channel = serial_buffer[0] & 0x0F;
    uint8_t cc      = serial_buffer[1];
    uint8_t val     = serial_buffer[2];
    
    Serial.println("Accepted CC");
    Serial.print(channel); Serial.print(" : ");
    Serial.print(cc); Serial.print(" : ");
    Serial.println(val);

    if (cc == datastore::configs.led_cc) {
      if (harddefs::channels_count <= channel) {
        Serial.print("Rejected Invalid Channel : "); Serial.println(channel);
        return;
      }
      if (val < 0 || datastore::led::state::Count <= val) {
        Serial.print("Rejected Invalid Value : "); Serial.println(val);
        return;
      }
      datastore::globals.leds[channel].set().s = static_cast<datastore::led::state>(val);
    }
    else if (cc == datastore::configs.expression_cc) {
      if (harddefs::exprs_count <= channel) {
        Serial.print("Rejected Invalid Channel : "); Serial.println(channel);
        return;
      }
      if (val < 0 || datastore::led::state::Count <= val) {
        Serial.print("Rejected Invalid Value : "); Serial.println(val);
        return;
      }
      datastore::globals.exprs[channel].set().s = static_cast<datastore::expr::state>(val);
    }
  }
  void process_sysex() {
    Serial.println("AcceptedSysex");
  }

  /** Parsing methods **/
  
  void accept_first_byte(uint8_t b) {
    
    serial_buffer.clear();
    if ((b & 0xF0) == 0xC0) {
      serial_buffer.push_back(b);
      serial_status = status::ReceivingCC;
    }
    else if (b == 0xF0) {
      serial_buffer.push_back(b);
      serial_status = status::ReceivingSysex;
    }
    else {
      serial_status = status::Rejected;
    }
  }
  
  void accept_cc_byte(uint8_t b) {
    if (b & 0x80) {
      accept_first_byte(b);
      /** TODO notify error **/
    }
    else {
      serial_buffer.push_back(b);
      if (serial_buffer.size == 3) {
        process_cc();
        serial_status = status::Ok;
      }
    }
  }

  void accept_sysex_byte(uint8_t b) {
    if ((b & 0x80) && b != 0xF7) {
      accept_first_byte(b);
      /** TODO notify error **/
    }
    else {
      if (serial_buffer.is_full()) {
        /** TODO notify error **/
        serial_status = status::Rejected;
      }
      serial_buffer.push_back(b);
      if (b == 0xF7) {
        process_sysex();
        serial_status = status::Ok;
      }
    }
  }
  
  void reject_byte(uint8_t b) {
    if (b & 0x80) {
      accept_first_byte(b);
    }
    else {
      /** TODO notify error **/
    }
  }

  void process_serial_in() {
    while (0 < Serial.available()) {
      /* get input */
      int rb = Serial.read();
      if (rb < 0) {
        continue;
      }
      /* process it */
      else {
        uint8_t b = rb;
//        Serial.print("Accept byte : ");
//        Serial.print((int)rb);
//        Serial.print(" : ");
//        Serial.println((int)b);
        switch (serial_status) {
          
          case status::Ok :
            accept_first_byte(b);
            break;
            
          case status::ReceivingCC :
            accept_cc_byte(b);
            break;
            
          case status::ReceivingSysex :
            accept_sysex_byte(b);
            break;
            
          case status::Rejected :
            reject_byte(b);
            break;
        }
      }
    }
  }
}

void setup() {
  
  /** Hardware setup **/
  
  Serial.begin(9600);
  
  datastore::globals.init(&datastore::configs);
  datastore::globals.dump();

  io::serial_raw_storage.fill([](int){ return 0; });
  io::serial_buffer.clear();
  io::serial_status = io::status::Ok;

  /** beautiful animation **/

}

void loop() {
//*
  datastore::globals.begin_frame();
  io::process_serial_in();
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
