#pragma once

#include <vector>
#include <utility>
#include <cstdint>
#include <cstddef>
#include <optional>
#include <variant>
#include <cassert>
#include <functional>
#include <unordered_map>

namespace sfx {
namespace io {

template <typename Object>
class parser {
public:

    using object_type = Object;
    using raw_type = std::vector<std::byte>;
    using raw_citerator = raw_type::const_iterator;

    /** Nested types **/
    struct message {
        enum types : uint8_t {
            Unkown = 0x00,
            Sysex = 0xF0,
        };
        types type;
        std::vector<std::byte> payload;
    };

    enum class status { Dead, Idle, Running };
    enum class code { Ok, InvalidHeader, InvalidPayload, Incomplete };

    using result = std::pair<std::size_t, std::optional<Object>>;
    using validator = std::function<
            result(raw_citerator begin, raw_citerator end)
        >;

    using protocol = std::unordered_map<std::byte, validator>;

    /** Ctors **/
    explicit parser(
        const protocol& p = protocol(),
        std::size_t hint = sizeof(Object))
        : _protocol(p), _buffer()
        { _buffer.reserve(hint); }

    /** Accessors **/
    const protocol& cfg() const { return _protocol; }
    status state() const 
    {
        if (_protocol.size() == 0)
            return status::Dead;
        else
            return is_running()
                ? status::Running
                : status::Idle
                ;
    }

    bool is_running() const { return 0 != _buffer.size(); }

    /** Methods **/
    std::vector<Object>
        operator() (const std::vector<std::byte>& v)
    {
        assert(state() != status::Dead);
        _buffer.insert(_buffer.end(), v.cbegin(), v.cend());
    
        std::vector<Object> res;
        buffer_iterator pos(_buffer.cbegin());
        while (pos != _buffer.cend()) {
            auto [itr, var] = try_parse(pos, _buffer.cend());

            if (var.index() == 0)
            {
                if (std::get<code>(var) == code::Incomplete)
                    break;
            }
            else
                res.emplace_back(std::get<Object>(var));
            pos = itr;
        }

        auto tmp = buffer_type(pos, _buffer.cend());
        std::swap(_buffer, tmp);

        return res;
    }

private:
    using buffer_type = std::vector<std::byte>;
    using buffer_iterator = buffer_type::const_iterator;
    
    std::pair<buffer_iterator, std::variant<code, Object>>
        try_parse(raw_citerator begin, raw_citerator end) const
    {
        assert(is_running());

        auto itr = _protocol.find(*begin);
        if (itr == _protocol.end())
            { return {begin+1, code::InvalidHeader}; }

        auto [len, res] = itr->second(begin, end);
        
        if (res.has_value())
            return {raw_citerator(begin+len), res.value()};
        else
            if (len == 0)
                return {begin, code::Incomplete};
            else
                return {begin+len, code::InvalidPayload};
    }

    protocol    _protocol;
    buffer_type _buffer;
};

} /**< namespace io **/
} /**< namespace sfx **/