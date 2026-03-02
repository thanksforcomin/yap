#pragma once

#include "audio_input.hpp"
#include "src/utils.hpp"
#include <libavformat/avformat.h>
#include <libavutil/dict.h>

namespace audio {
  template <AudioSubscriber... Subscribers>
  AudioInput<Subscribers...>::AudioInput(auto &&options, auto &&device_url,
                                         auto &&input_format_name,
                                         Subscribers &&...subscribers)
    : subscribers(std::forward<Subscribers>(subscribers)...),
      fmt_ctx_ptr(nullptr) {}
  
  template <AudioSubscriber... Subscribers>
  auto AudioInput<Subscribers...>::makeInput(
      utils::formattable auto &&device_url,
      utils::formattable auto &&input_format_name)
      -> Result<AVFormatContext *> {
    AVInputFormat *input_format = av_find_input_format(input_format_name);
    
    if (!input_format) {
      utils::report_error("Unknown input format: {}", input_format_name);
      return std::unexpected(UnknownInputFormat);
    }

    AVFormatContext *fmt_context = nullptr;
    AVDictionary *options = nullptr;

    setOptions(options);

    if (auto ret = avformat_open_input(&fmt_context, device_url,
                                       input_format_name, &options);
        ret < 0) {
      utils::report_error(ret, "Failed to open device");
      return std::unexpected(FailedToOpenDevice);
    }

    return fmt_context;
  }

  // sample here, we need to actually store them in some dictionary
  template <AudioSubscriber... Subscribers>
  auto AudioInput<Subscribers...>::setOptions(AVDictionary *options)
      -> void {
    av_dict_set(&options, "sample_rate", "44100", 0);
  }

  template <AudioSubscriber... Subscribers>
  AudioInputBuilder<Subscribers...>::AudioInputBuilder(
      Subscribers &&...subscribers)
      : options(), subs(std::forward<Subscribers>(subscribers)...) {}

  template <AudioSubscriber... Subscribers>
  auto AudioInputBuilder<Subscribers...>::setOption(auto &&key, auto &&value)
      -> AudioInputBuilder & {
    auto opts = options.get();
    av_dict_set(&opts, key, value, 0);

    return *this;
  }

  template <AudioSubscriber... Subscribers>
  auto AudioInputBuilder<Subscribers...>::setDeviceUrl(auto &&value)
      -> AudioInputBuilder & {
    device_url = value;
    
    return *this;
  }
  
  template <AudioSubscriber... Subscribers>
  auto AudioInputBuilder<Subscribers...>::setInputFormat(auto &&value)
      -> AudioInputBuilder & {
    input_format = value;
    
    return *this;
  }
  
  template <AudioSubscriber... Subscribers>
  auto AudioInputBuilder<Subscribers...>::build()
      -> AudioInput<Subscribers...> {
    return std::apply([&](Subscribers &&...subscribers) {
      return AudioInput(options, device_url, input_format,
                        std::forward<Subscribers>(subscribers)...);
    });
  }
}
