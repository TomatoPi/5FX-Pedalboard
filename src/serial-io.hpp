#pragma once

#include <string>
#include <vector>
#include <utility>
#include <memory>

namespace sfx {
  namespace io {

    class serial {
    public:

      /** Nested types **/
      struct config {
        int         baudrate;
        std::string port;

        explicit operator bool() const
          { return baudrate != 0 && port.size() != 0; }
      };
      
      enum class status { Dead, Active };
      enum class result { Ok, Failed, Truncated };

      /** Accessors **/
      config cfg() const { return _cfg; }
      status state() const
      { 
        return bool(_config) && bool(_handle)
          ? status::Active
          : status::Dead
          ;
      }

      /** Methods **/
      result begin(config cfg = _cfg);
      void end();

      std::pair<result, ssize_t>
      send(const std::vector<std::byte>& msg);
      
      std::pair<result, std::vector<std::byte>>
      receive(size_t hint = 16);
      
      result flush();

    private:
      
      class handle {
      public:
        ~handle();
        static std::optional<handle> try_open(config cfg);

        handle(const handle&) = delete;
        handle& operator= (const handle&) = delete;
        
        handle(handle&& s);
        handle& operator= (handle&& s);

        int fd() const { return _fd; }
        explicit operator bool() const { return _fd != 0; }

      private:
        handle(int fd) : _fd{fd} {}

        int _fd;
      };

      config                  _cfg;
      std::unique_ptr<handle> _handle;
    };
  }
}