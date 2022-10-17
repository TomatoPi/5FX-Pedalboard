#include "datastore.hpp"
#include "io.hpp"

namespace datastore
{

  void global::init(const config *cfg)
  {
    this->cfg = cfg;
    /* retrieve initial values */
    switches.fill([](int16_t i)
                  {
          int pin = harddefs::switch_pin(i);
          pinMode(pin, INPUT_PULLUP);
          return footswitch{i, static_cast<footswitch::state>(digitalRead(pin))}; });
    exprs.fill([](int16_t i)
               { return expr{i, expr::state::Disabled,
                             static_cast<uint16_t>(analogRead(harddefs::expr_pin(i)))}; });
    leds.fill([](int16_t i)
              {
          pinMode(harddefs::led_pin(i), OUTPUT);
          return led{i, led::state::Off}; });
    debounce_sw_timers.fill([](int16_t)
                            { return millis(); });
    /* update */
    // push_changes();
  }

  void global::begin_frame()
  {
    for (auto &sw : switches)
      sw.reset_changed();
    for (auto &led : leds)
      led.reset_changed();
    for (auto &ex : exprs)
      ex.reset_changed();
  }

  void global::read_inputs()
  {
    /** Switches **/
    unsigned long t = millis();
    for (auto &sw : switches)
    {
      /** Pass during debouncing **/
      if ((t - debounce_sw_timers[sw.get().id]) < cfg->debounce_sw_duration)
        continue;

      auto v = digitalRead(harddefs::switch_pin(sw.get().id));
      auto s = v == HIGH ? footswitch::state::Pressed : footswitch::state::Released;
      if (s != sw.get().s)
      {
        sw.set().s = s;
        debounce_sw_timers[sw.get().id] = t;
      }
    }
    /** Expressions **/
    for (auto &ex : exprs)
    {
      if (ex.get().s == expr::state::Disabled)
        continue;
      auto v = static_cast<uint16_t>(analogRead(harddefs::expr_pin(ex.get().id)));
      if (v != ex.get().value)
        ex.set().value = v;
    }
  }

  void global::push_changes() const
  {
    for (const auto &sw : switches)
    {
      if (!sw.changed())
        continue;
      io::write(0xC0 | sw.get().id, cfg->footswitch_cc, sw.get().s);
      io::print("SW changed : ", sw.get().id, " : ", static_cast<int>(sw.get().s));
      // digitalWrite(harddefs::led_pin(sw.get().id), static_cast<int>(sw.get().s));
    }
    for (const auto &l : leds)
    {
      if (!l.changed())
        continue;
      int state = l.get().s == led::state::On ? HIGH : LOW;
      digitalWrite(harddefs::led_pin(l.get().id), state);
    }
    for (const auto &ex : exprs)
    {
      if (!ex.changed())
        continue;
      /* convert 10 bits value to 14 bits */
      uint16_t val16 = ex.get().value << (14 - 10);
      uint8_t msb = (val16 & (0x007F << 7)) >> 7;
      uint8_t lsb = (val16 & 0x007F);
      io::write(0xC0 | ex.get().id, cfg->expression_cc, msb);
      io::write(0xC0 | ex.get().id, cfg->expression_cc + 0x20, lsb);
      io::print("Expr changed : ", ex.get().id, " : ", static_cast<int>(ex.get().value));
    }
  }

  void global::dump() const
  {
    for (const auto &sw : switches)
    {
      io::print("SW : ", sw.get().id,
                " : ", harddefs::switch_pin(sw.get().id),
                " : ", debounce_sw_timers[sw.get().id],
                " : ", static_cast<int>(sw.get().s));
    }
    for (const auto &led : leds)
    {
      io::print("Led : ", led.get().id, " : ", static_cast<int>(led.get().s));
    }
    for (const auto &ex : exprs)
    {
      io::print("Expr : ", ex.get().id, " : ", static_cast<int>(ex.get().value));
    }
  }
}
