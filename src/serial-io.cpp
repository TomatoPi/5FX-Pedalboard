#include "serial-io.hpp"

#include <stdio.h>    // Standard input/output definitions 
#include <unistd.h>   // UNIX standard function definitions 
#include <fcntl.h>    // File control definitions 
#include <errno.h>    // Error number definitions 
#include <termios.h>  // POSIX terminal control definitions 
#include <string.h>   // String function definitions 
#include <sys/ioctl.h>

#include <cassert>

namespace {
  // takes the string name of the serial port (e.g. "/dev/tty.usbserial","COM1")
  // and a baud rate (bps) and connects to that port at that speed and 8N1.
  // opens the port in fully raw mode so you can send binary data.
  // returns valid fd, or -1 on error
  int serialport_init(const char* serialport, int baud)
  {
      struct termios toptions;
      int fd;
      
      //fd = open(serialport, O_RDWR | O_NOCTTY | O_NDELAY);
      fd = open(serialport, O_RDWR | O_NONBLOCK );
      
      if (fd == -1)  {
          perror("serialport_init: Unable to open port ");
          return -1;
      }
      
      //int iflags = TIOCM_DTR;
      //ioctl(fd, TIOCMBIS, &iflags);     // turn on DTR
      //ioctl(fd, TIOCMBIC, &iflags);    // turn off DTR

      if (tcgetattr(fd, &toptions) < 0) {
          perror("serialport_init: Couldn't get term attributes");
          return -1;
      }
      speed_t brate = baud; // let you override switch below if needed
      switch(baud) {
      case 4800:   brate=B4800;   break;
      case 9600:   brate=B9600;   break;
  #ifdef B14400
      case 14400:  brate=B14400;  break;
  #endif
      case 19200:  brate=B19200;  break;
  #ifdef B28800
      case 28800:  brate=B28800;  break;
  #endif
      case 38400:  brate=B38400;  break;
      case 57600:  brate=B57600;  break;
      case 115200: brate=B115200; break;
      }
      cfsetispeed(&toptions, brate);
      cfsetospeed(&toptions, brate);

      // 8N1
      toptions.c_cflag &= ~PARENB;
      toptions.c_cflag &= ~CSTOPB;
      toptions.c_cflag &= ~CSIZE;
      toptions.c_cflag |= CS8;
      // no flow control
      toptions.c_cflag &= ~CRTSCTS;

      //toptions.c_cflag &= ~HUPCL; // disable hang-up-on-close to avoid reset

      toptions.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
      toptions.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

      toptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
      toptions.c_oflag &= ~OPOST; // make raw

      // see: http://unixwiz.net/techtips/termios-vmin-vtime.html
      toptions.c_cc[VMIN]  = 0;
      toptions.c_cc[VTIME] = 0;
      //toptions.c_cc[VTIME] = 20;
      
      tcsetattr(fd, TCSANOW, &toptions);
      if( tcsetattr(fd, TCSAFLUSH, &toptions) < 0) {
          perror("init_serialport: Couldn't set term attributes");
          return -1;
      }

      return fd;
  }

}

namespace sfx {
  namespace io {

    serial::result serial::begin(config cfg)
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
        return {result::Failed, n};
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
      if (-1 == tcflush(_handle->fd(), TCIOFLUSH))
        return result::Failed;
      else
        return result::Ok;
    }

    void serial::handle::terminate()
    {
      if (bool(*this))
        close(_fd);
    }
    std::optional<serial::handle>
      serial::handle::try_open(config cfg)
    {
      int fd = serialport_init(cfg.port.c_str(), cfg.baudrate);
      if (fd <= 0)
        return std::nullopt;
      else
        return std::make_optional<handle>(fd);
    }
  }
}