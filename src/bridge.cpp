#include "arduino-serial-lib.h"
#include <termios.h>
#include <iostream>
#include <cstdint>
#include <unistd.h>
#include <vector>


void usage()
{

}

int main(int argc, char * const argv[])
{
    if (argc != 2)
    {
        usage();
        return -1;
    }

    int serialfd = serialport_init(argv[1], B9600);
    if (serialfd < 0)
    {
        std::cerr << "Failed open serial : " << argv[1] << std::endl;
    }

    usleep(1000000);

    while (true) {
        char ibuffer[256];
        int err;
        if (-1 == (err = serialport_read_until(serialfd, ibuffer, '\n', 256, 1000)))
        {
            std::cout << "Failed read serial : " << err << std::endl;
            return -1;
        }
        if (*ibuffer != 0) {
            std::cout << "XX : " << ibuffer;
        } else {
            std::vector<std::uint8_t> obuffer = {0xC0, 0x03, 0x01, 0xC1, 0x0B, 0x01};
            // std::vector<std::uint8_t> obuffer = {0xF0, 0x00, 0x00, 0xF7};
            for (std::uint8_t x : obuffer) {
                int err;
                // std::cout << "Send : " << std::hex << (int)x << std::endl;
                if (0 != (err = serialport_writebyte(serialfd, x))) {
                    std::cerr << "Failed write serial : " << err << std::endl;
                    return -1;
                }
                usleep(1000);
            }
            serialport_flush(serialfd);
            usleep(1000000);
        }
    }

    return 0;
}