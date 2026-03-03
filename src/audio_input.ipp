#pragma once

#include "audio_input.hpp"
#include "src/utils.hpp"
#include <cerrno>
#include <print>

namespace audio {
  template <AudioSubscriber... Subscribers>
  AudioInput<Subscribers...>::AudioInput(auto &&options, auto &&device_url,
                                         auto &&input_format_name,
                                         Subscribers &...subscribers)
    : subscribers(std::tie(subscribers...)),
      fmt_ctx_ptr(*makeInput(options, device_url, input_format_name)),
      audio_index(-1) {
    if (!setup()) {
      utils::report_error("Cannot build audio input");
    }

    AudioInputInfo info {
      .input_params = fmt_ctx_ptr->streams[audio_index]->codecpar;
    };
    
    std::apply([&](Subscribers &...subs) { (subs.updateAudioInfo(info), ...); },
               this->subscribers);
  }

  template <AudioSubscriber... Subscribers>
  auto AudioInput<Subscribers...>::makeInput(
      AVDictionary *options, utils::formattable auto &&device_url,
      utils::formattable auto &&input_format_name)
      -> Result<AVFormatContext *> {
    AVInputFormat *input_format =
        const_cast<AVInputFormat *>(av_find_input_format(input_format_name));
    
    if (!input_format) {
      utils::report_error(std::format("Unknown input format: {}", input_format_name));
      return std::unexpected(UnknownInputFormat);
    }

    AVFormatContext *fmt_context = nullptr;

    if (auto ret = avformat_open_input(&fmt_context, device_url,
                                       input_format, &options);
        ret < 0) {
      utils::report_error(ret, "Failed to open device");
      return std::unexpected(FailedToOpenDevice);
    }
    
    return fmt_context;
  }

  template <AudioSubscriber... Subscribers>
  auto AudioInput<Subscribers...>::setup() -> Result<void> {
    if (auto ret = avformat_find_stream_info(fmt_ctx_ptr.get(), nullptr);
        ret < 0) {
      utils::report_error("Could not find stream info");
      return std::unexpected(CannotFindStreamInfo);
    }

    for (int32_t i = 0; i < static_cast<int>(fmt_ctx_ptr->nb_streams); ++i) {
      if (fmt_ctx_ptr->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        audio_index = i;
        break;
      }
    }

    if (audio_index == -1) {
      utils::report_error("No audio stream found");
      return std::unexpected(NoAudioStreamFound);
    }

    return {};
  }

  template <AudioSubscriber... Subscribers>
  auto AudioInput<Subscribers...>::run() -> void {
    std::println("Capturing audio... Press Ctrl+C to abort");

    PacketWrapper pack;

    while (true) {
      if (auto ret = av_read_frame(fmt_ctx_ptr.get(), &pack.pack); ret < 0) {
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
          break;

        utils::report_error(ret, "Error reading frame: ");
        break;
      }

      if (pack.pack.stream_index == audio_index) {
        std::apply([&](Subscribers&... subs) {
          (subs.process(pack),...);
        }, subscribers);
      }
    }
  }

  template <AudioSubscriber... Subscribers>
  AudioInputBuilder<Subscribers...>::AudioInputBuilder(
      Subscribers &...subscribers)
    : options(), subs(std::tie(subscribers...)) {}

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
    return std::apply([&](Subscribers &...subscribers) {
      return AudioInput(options.get(), device_url.data(), input_format.data(),
                        subscribers...);
    }, subs);
  }
}
