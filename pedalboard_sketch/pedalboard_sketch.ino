
#include <Arduino.h>
#include <EEPROM.h>

#include "harddefs.hpp"
#include "array.hpp"
#include "datastore.hpp"
#include "io.hpp"

namespace datastore {
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
    
    io::print("Accepted CC : ", channel, " : ", cc, " : ", val);

    if (cc == datastore::configs.led_cc) {
      if (harddefs::channels_count <= channel) {
        io::print("Rejected Invalid Channel : ", channel);
        return;
      }
      if (val < 0 || datastore::led::state::Count <= val) {
        io::print("Rejected Invalid Value : ", val);
        return;
      }
      datastore::globals.leds[channel].set().s = static_cast<datastore::led::state>(val);
    }
    else if (cc == datastore::configs.expression_cc) {
      if (harddefs::exprs_count <= channel) {
        io::print("Rejected Invalid Channel : ", channel);
        return;
      }
      if (val < 0 || datastore::led::state::Count <= val) {
        io::print("Rejected Invalid Value : ", val);
        return;
      }
      datastore::globals.exprs[channel].set().s = static_cast<datastore::expr::state>(val);
    }
  }
  void process_sysex() {
    io::print("AcceptedSysex");
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
  // datastore::globals.dump();

  io::serial_raw_storage.fill([](int){ return 0; });
  io::serial_buffer.clear();
  io::serial_status = io::status::Ok;

  /** beautiful animation **/

  /** Send presentation */
  
  io::present("5FX-Pedalboard:001");
}

void loop() {
//*
  datastore::globals.begin_frame();
  io::process_serial_in();
  datastore::globals.read_inputs();
  datastore::globals.push_changes();
  Serial.flush();
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
