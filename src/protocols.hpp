#pragma once

#include <functional>

namespace mycelium {
namespace io {

struct protocol {

  using accept = typename accept<range>::return_type;
  using reject = typename reject<range>::return_type;

  using result = std::variant<accept, reject>
  using range  = std::pair<iterator, iterator>;
  using return_type = std::pair<result, range>;
  
  auto operator() (range)
};

struct acceptator<predicate> {

  return_type operator() (range) const {

  }
};

struct interpreter {
  struct reject {};
  struct accept {};
  /**
  Consume a range of tokens, return a new interpreter
  **/
  auto operator() (range, state) const
    { return std::invoke(state, range, state); }
};

struct mapper {
  struct table {};
  /**
  Consume a range of tokens, return a new interpreter
  **/
  auto operator() (range, table) const
  {
    const auto& table = std::get<table>(state);
    auto citr = std::find(table, range.begin());
    if (citr == table.cend())
      return interpreter::reject(range.begin(), itr);
    
    auto [itr, citr] = interpreter()(range, table);
    else
      return interpreter::accept(range.begin(), itr, citr);
  }
};

struct buffered<state, buffer> {}
  auto operator() (range, state, buffer) const
    {
      return std::invoke(
        std::mem_fn(buffer, &buffer::consume),
        interpreter()(range, state)
        );
    }
}

}