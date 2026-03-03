#pragma once

#include <concepts>
#include <format>
#include <type_traits>
#include <print>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
}

namespace utils {
template <typename T>
concept integral = std::integral<std::remove_reference_t<T>>;

  template <typename T>
  using element_type = std::decay_t<decltype(std::declval<std::decay_t<T>>()[0])>;

  template <typename T>
  concept formattable = std::formattable<T, element_type<T>>;


  void report_error(utils::integral auto&& code, utils::formattable auto&& message) {
    static std::array<char, 128> errbuf;
    av_strerror(code, errbuf.data(), sizeof(errbuf));
    std::println("ERROR: {}: {}", message, errbuf.data());
  }
  
  void report_error(utils::formattable auto &&message) {
    std::print("ERROR: {}", message);
  }
}
