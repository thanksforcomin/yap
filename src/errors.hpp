#pragma once

#include <cstdint>
#include <expected>
#include <type_traits>
#include <utility>

namespace errors {
  enum Errors : uint64_t { UnknownError = 1 << 0 };

  template <typename T> using Result = std::expected<T, Errors>;

  constexpr auto operator|(Errors a, Errors b) -> Errors {
    return static_cast<Errors>(std::to_underlying(a) | std::to_underlying(b));
  }


  template <typename ErrorType>
  constexpr auto operator|(ErrorType accum,
                           std::expected<void, ErrorType> &&res) -> ErrorType {
    if (res.has_value())
      return accum;
    else
      return accum | res.error();
  }
}
