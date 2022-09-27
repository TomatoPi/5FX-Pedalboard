#include "serial-io.hpp"

#include <stdio.h>    // Standard input/output definitions 
#include <unistd.h>   // UNIX standard function definitions 
#include <fcntl.h>    // File control definitions 
#include <errno.h>    // Error number definitions 
#include <termios.h>  // POSIX terminal control definitions 
#include <string.h>   // String function definitions 
#include <sys/ioctl.h>

#include <cassert>

namespace sfx {
  namespace io {

      serial::result serial::begin(config cfg /* = _cfg */)
      {
        if (!cfg) return result::Failed;
        auto h = handle::try_open(cfg);
        if (!h.has_value()) return result::Failed;
        _handle = std::make_unique<handle>(std::move(h.value()));
        _cfg = cfg;

        assert(status::Active == state());
        return result::Ok;
      }
      void serial::end()
      {
        _handle.reset();
        assert(status::Dead == state());
      }

      std::pair<serial::result, ssize_t>
      serial::send(const std::vector<std::byte>& msg)
      {
        assert(status::Active == state());
        ssize_t n = write(_handle->fd(), msg.data(), msg.size());
        if (n < 0)
          return {result::Failure, n};
        else if (static_cast<size_t>(n) != msg.size())
          return {result::Truncated, n};
        else
          return {result::Ok, 0};
      }
      
      std::pair<serial::result, std::vector<std::byte>>
      serial::receive(size_t hint /* = 16 */)
      {
        assert(status::Active == state());
        std::vector<std::byte> buffer;
        buffer.resize(hint);

        ssize_t n = 0;
        size_t written = 0;
        while (0 < (n = read(_handle->fd(), buffer.data() + written, hint)))
        {
          written += n;
          if (n != hint) break;
          /**< less than hint bytes read = no more available */
          if (buffer.capacity() < written + hint)
            buffer.resize(buffer.capacity() * 2);
        }
        buffer.resize(written);

        if (n == -1 && errno != EWOULDBLOCK && errno == EAGAIN)
          return {result::Failed, buffer};
        else
          return {result::Ok, buffer};
      }
      
      serial::result serial::flush()
      {
        assert(status::Active == state());
        if (-1 == tcflush(_handel->fd(), TCIOFLUSH))
          return result::Failed;
        else
          return result::Ok;
      }

  }
}