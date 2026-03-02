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
  AudioInputBuilder::AudioInputBuilder() : options()
}
