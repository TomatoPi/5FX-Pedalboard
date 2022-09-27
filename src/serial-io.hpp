#pragma once

#include <string>
#include <vector>
#include <utility>
#include <optional>
#include <memory>

namespace sfx {
  namespace io {

    class serial {
    public:

      /** Nested types **/
      struct config {
        std::string port;
        int         baudrate;

        explicit operator bool() const
          { return baudrate != 0 && port.size() != 0; }
      };
      
      enum class status { Dead, Active };
      enum class result { Ok, Failed, Truncated };

      /** Accessors **/
      config cfg() const { return _cfg; }
      status state() const
      { 
        return bool(_cfg) && bool(_handle)
          ? status::Active
          : status::Dead
          ;
      }

      /** Methods **/
      result begin(config cfg);
      void end();

      std::pair<result, ssize_t>
      send(const std::vector<std::byte>& msg);
      
      std::pair<result, std::vector<std::byte>>
      receive(size_t hint = 16);
      
      result flush();

    private:
      
      class handle {
      public:
        /** C calls wrappers **/
        static std::optional<handle>
          try_open(config cfg);

        void terminate();

        /** Ctors **/
        handle(const handle&) = delete;
        handle& operator= (const handle&) = delete;
        
        handle(handle&& s)
          : _fd{s._fd}
          { s._fd = 0; }
        handle& operator= (handle&& s)
          { terminate(); _fd = s._fd; return *this; }

        ~handle() { terminate(); }

        /** Accessors **/
        int fd() const
          { return _fd; }

        explicit operator bool() const
          { return _fd != 0; }

      private:
        handle(int fd) : _fd{fd} {} /**< Prevent invalid handle **/
        int _fd;                    /**< File descriptor **/
      };

      config                  _cfg;
      std::unique_ptr<handle> _handle;
    };
  }
}