#pragma once

#include "audio_input.hpp"
#include "src/utils.hpp"
#include <cerrno>
#include <libavcodec/codec_par.h>
#include <libavformat/avformat.h>
#include <print>
#include <utility>

namespace audio {
  inline auto AudioInput::init(auto &&options, auto &&device_url,
                                        auto &&input_format_name)
    -> Result<AudioInput> {
    auto format_context = makeInput(options, device_url, input_format_name);
    
    if (!format_context) 
      return std::unexpected(format_context.error());

    auto audio_index = getAudioStream(*format_context);
    
    if (!audio_index)
      return std::unexpected(audio_index.error());

    return AudioInput(*format_context, *audio_index);
  }     

  AudioInput::AudioInput(auto &&format_context, auto &&audio_index)
      : audio_index(audio_index),
        fmt_ctx_ptr(std::forward<decltype(format_context)>(format_context)) {}

  inline auto AudioInput::makeInput(AVDictionary *options,
                             utils::formattable auto &&device_url,
                             utils::formattable auto &&input_format_name)
    -> Result<AVFormatContext *> {
    AVInputFormat *input_format =
      const_cast<AVInputFormat *>(av_find_input_format(input_format_name));
    
    if (!input_format) {
      utils::report_error(std::format("Unknown input format: {}", input_format_name));
      return std::unexpected(UnknownInputFormat);
    }
    
    AVFormatContext *fmt_context = nullptr;

    if (auto ret = avformat_open_input(&fmt_context, device_url, input_format,
                                       &options);
        ret < 0) {
      utils::report_error(ret, "Failed to open device");

      avformat_close_input(&fmt_context);
      
      return std::unexpected(FailedToOpenDevice);
    }
    
    return fmt_context;
  }

  inline auto AudioInput::getAudioStream(auto&& format_ctx) -> Result<int> {
    int audio_stream_index = -1;
    
    if (auto ret = avformat_find_stream_info(format_ctx, nullptr);
        ret < 0) {
      utils::report_error("Could not find stream info");
      return std::unexpected(CannotFindStreamInfo);
    }

    for (int32_t i = 0; i < static_cast<int>(format_ctx->nb_streams); ++i) {
      if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        audio_stream_index = i;
        break;
      }
    }

    if (audio_stream_index == -1) {
      utils::report_error("No audio stream found");
      return std::unexpected(NoAudioStreamFound);
    }

    return audio_stream_index;
  }

  inline auto AudioInput::getCodecParams() const
      -> AVCodecParameters * {
    return fmt_ctx_ptr->streams[audio_index]->codecpar;
  }


  inline auto AudioInputBuilder::setOption(auto &&key, auto &&value)
      -> AudioInputBuilder & {
    auto opts = options.get();
    av_dict_set(&opts, key, value, 0);

    return *this;
  }

  inline auto AudioInputBuilder::setDeviceUrl(auto &&value)
      -> AudioInputBuilder & {
    device_url = value;
    
    return *this;
  }
  
  inline auto AudioInputBuilder::setInputFormat(auto &&value)
      -> AudioInputBuilder & {
    input_format = value;
    
    return *this;
  }

  inline auto AudioInputBuilder::build() -> Result<AudioInput> {
    return AudioInput::init(options.get(), device_url.data(), input_format.data());
  }
}
