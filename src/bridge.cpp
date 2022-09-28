#include "serial-io.hpp"
#include "engine.hpp"
#include <termios.h>
#include <iostream>
#include <cstdint>
#include <unistd.h>
#include <vector>
#include <string>
#include <sstream>

#include <error.h>

void usage()
{
    std::cout << "5FX-Pedalboard port baudrate" << std::endl;
}

int main(int argc, char * const argv[])
{
    if (argc != 3)
    {
        usage();
        return -1;
    }

    using namespace sfx;

    io::serial::config config;
    {
        config.port = argv[1];
        std::stringstream ss;
        ss << argv[2];
        ss >> config.baudrate;
    }

    io::serial serial;
    if (config && io::serial::result::Ok != serial.begin(config))
    {
        std::cerr << "Failed open port" << std::endl;
        perror("");
        return -1;
    }

    /** Let the arduino wake up **/
    usleep(1000000);

    /** MAIN LOOP **/
    while (io::serial::status::Active == serial.state())
    {
        {
            auto [code, msg] = serial.receive(64);
            if (io::serial::result::Ok != code)
            {
                std::cerr << "Receive failure" << std::endl;
                perror("");
                serial.end();
                break;
            }
            /** DEBUG **/
            std::cout << "Received ";
            for (auto x : msg) std::cout << char(x) << ' ';
            std::cout << '\n';
        }

        {
            uint8_t omsg[] = {0xC0, 0x03, 0x01, 0xC1, 0x0B, 0x01};
            std::vector<std::byte> adaptor(
                reinterpret_cast<std::byte*>(&omsg[0]),
                reinterpret_cast<std::byte*>(&omsg[sizeof(omsg)])
                );
            auto [code, len] = serial.send(adaptor);
        }

        usleep(1000000);
    }
    return 0;
}
