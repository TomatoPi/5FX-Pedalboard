#pragma once

#include <Arduino.h>
#include <stdint.h>

namespace harddefs
{
  /** number of inputs on the pedalboard */
  static constexpr const uint8_t channels_count = 8;

  /** map between software indices and hardware wiring */
  static constexpr const unsigned int leds_pins[channels_count] = {6, 7, 0, 1, 5, 4, 3, 2};
  static constexpr const unsigned int switches_pins[channels_count] = {A3, A4, 10, 11, A2, A1, A0, 12};

  /** returns hardware pin of given io */
  constexpr int led_pin(unsigned int i) { return 2 + leds_pins[i]; }
  constexpr int switch_pin(unsigned int i) { return switches_pins[channels_count - i - 1]; }

  /** number of expression pedals */
  static constexpr const uint8_t exprs_count = 2;

  /** map between soft and hardware */
  static constexpr const unsigned int exprs_pins[exprs_count] = {A6, A7};

  /** return hardware pin of given expression pedal */
  constexpr int expr_pin(unsigned int i) { return exprs_pins[i]; }

  /** Other constants **/
  static constexpr const size_t serial_buffer_size = 128;

  static constexpr const size_t inputs_count = channels_count + exprs_count;
  static constexpr const size_t outputs_count = channels_count + exprs_count;
}
