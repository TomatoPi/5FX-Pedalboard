#include "serial-io.hpp"
#include "parser.hpp"
#include <termios.h>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#include <unistd.h>
#include <error.h>

// #define __ENABLE_TESTING__

void usage()
{
    std::cout << "5FX-Pedalboard port baudrate" << std::endl;
}

int main(int argc, char *const argv[])
{
    using namespace sfx;

#ifndef __ENABLE_TESTING__
    if (argc != 3)
    {
        usage();
        return -1;
    }

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
#else

    using object_type = int;
    using parser_type = io::parser<object_type>;
    using code = parser_type::code;
    using result = parser_type::result;
    using validator = parser_type::validator;
    using iterator = parser_type::raw_citerator;
    using protocol_type = parser_type::protocol;

    validator sysex = [](iterator begin, iterator end) -> result
    {
        auto itr = std::find_if(begin + 1, end, [](std::byte b) -> bool
                                { return bool(b & std::byte(0x80)); });
        if (itr == end)
            return result();
        if (*itr != std::byte(0xF7))
            return result(itr - begin, std::nullopt);
        else
            return result(
                itr - begin + 1,
                std::make_optional(static_cast<int>(*(begin + 1))));
    };
    validator cc = [](iterator begin, iterator end) -> result
    {
        if (end - begin < 3)
            return result();

        if (bool(*(begin + 1) & std::byte(0x80)))
            return result(1, std::nullopt);
        if (bool(*(begin + 2) & std::byte(0x80)))
            return result(2, std::nullopt);

        int channel = static_cast<int>(*begin & std::byte(0x0F));
        int cc = static_cast<int>(*(begin + 1));
        int value = static_cast<int>(*(begin + 2));

        return result(3, std::make_optional(channel));
    };

    protocol_type protocol{
        {std::byte(0xF0), sysex}};
    for (int i = 0xC0; i <= 0xCF; ++i)
        protocol.emplace(std::byte(i), cc);

    parser_type parser(protocol);

    uint8_t omsg[] = {0xF0, 0x15, 0xF7, 0xC0, 0x03, 0x01, 0xC1, 0x0B, 0x01};
    std::vector<std::byte> adaptor(
        reinterpret_cast<std::byte *>(&omsg[0]),
        reinterpret_cast<std::byte *>(&omsg[sizeof(omsg)]));
    auto vect = parser(adaptor);

    std::cout << "SIZE : " << vect.size() << std::endl;
    for (auto x : vect)
        std::cout << "SUCCESS : " << x << std::endl;

    return 0;

#endif
#ifndef __ENABLE_TESTING__
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
            if (msg.size() != 0)
            {
                if (3 < msg.size() && msg[0] == std::byte(0xF0) && msg[msg.size() - 1] == std::byte(0xF7))
                {
                    std::cout << "Sysex" << std::endl;
                }
                else
                {
                    std::cout << "Received ";
                    for (auto x : msg)
                        std::cout << char(x); // << ' ';
                    std::cout << '\n';
                }
            }
        }

        {
            uint8_t omsg[] = {0xC0, 0x03, 0x01, 0xC1, 0x0B, 0x00};
            std::vector<std::byte> adaptor(
                reinterpret_cast<std::byte *>(&omsg[0]),
                reinterpret_cast<std::byte *>(&omsg[sizeof(omsg)]));
            auto [code, len] = serial.send(adaptor);
        }

        usleep(1000000);
    }
    return 0;
#endif
}
