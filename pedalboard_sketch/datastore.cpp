#include "datastore.hpp"

namespace datastore {
  
    void global::init(const config* cfg) {
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

    void global::begin_frame() {
      for (auto& sw : switches) sw.reset_changed();
      for (auto& led: leds)     led.reset_changed();
      for (auto& ex : exprs)    ex.reset_changed();
    }

    void global::read_inputs() {
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

    void global::push_changes() const {
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

    void global::dump() const {
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
}
