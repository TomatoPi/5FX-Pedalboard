#pragma once

#include <string>
#include <vector>
#include <cassert>
#include <unordered_map>

namespace sfx {
    namespace datastore {

        struct entry {
        
            struct name_hash {
                std::size_t operator() (const entry& e) const
                    { return std::hash<std::string>()(e.name); }
            };
            struct name_equal {
                bool operator() (const entry& a, const entry& b) const
                    { return a.name == b.name; }
            };
          
            std::string            name;
            std::string            type;
            std::vector<std::byte> value;

            template <typename T> const T& get() const
            {
                assert(sizeof(T) == value.size());
                return reinterpret_cast<const T*>(value.data());
            }
            template <typename T> void set(const T& t)
            {
                value.resize(sizeof(T));
                *reinterpret_cast<T*>(value.data()) = t;
            }
        };

        using unordered_map = std::unordered_map<entry, entry::name_hash, entry::name_equal>;
    }
}