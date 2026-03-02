#pragma once

#include "src/utils.hpp"
#include <expected>
#include <libavcodec/packet.h>
#include <libavutil/channel_layout.h>
#include <memory>
#include <mutex>
#include <tuple>
#include <unordered_map>
extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
}

namespace audio {

  enum Errors {
    UnknownInputFormat,
    FailedToOpenDevice,
    CannotFindStreamInfo,
    NoAudioStreamFound
  };
  
  template <typename T>
  using Result = std::expected<T, Errors>;

  struct PacketWrapper {
    AVPacket pack;

    PacketWrapper();

    PacketWrapper(const PacketWrapper &other) = delete;
    auto operator=(const PacketWrapper &other) -> PacketWrapper & = delete;

    PacketWrapper(PacketWrapper &&other) = default; 
    auto operator=(PacketWrapper &&other) -> PacketWrapper & = default;
    
    ~PacketWrapper();
  };

  template <typename T>
  concept AudioSubscriber = requires(T t, PacketWrapper packet) {
    { t.process(std::move(packet)) } -> std::same_as<void>;
  };

  inline constexpr auto _formatContextDeleter = [](AVFormatContext *ctx) {
    if (ctx)
      avformat_close_input(&ctx);
  };
  
  template <AudioSubscriber... Subscribers> class AudioInput {
    std::tuple<Subscribers...> subscribers;

    std::unique_ptr<AVFormatContext, decltype(_formatContextDeleter)> fmt_ctx_ptr;
    
  public:
    AudioInput(auto&& options, auto &&device_url, auto &&input_format_name,
               Subscribers &&...subscribers);

    AudioInput(const AudioInput &) = delete;
    auto operator=(const AudioInput &) -> AudioInput & = delete;

    AudioInput(AudioInput &&other);
    auto operator=(AudioInput &&other) -> AudioInput &;

    ~AudioInput();

    auto run() -> void;

  private:
    auto makeInput(utils::formattable auto &&device_url,
                   utils::formattable auto &&input_format_name)
        -> Result<AVFormatContext *>;

    auto setOptions(AVDictionary *options) -> void;
  };

  inline constexpr auto _avDictionaryDeleter = [](AVDictionary *dict) {
    if (dict)
      av_dict_free(&dict);
  };
  
  // NOLINTNEXTLINE
  template <AudioSubscriber... Subscribers> struct AudioInputBuilder {
    std::unique_ptr<AVDictionary, decltype(_avDictionaryDeleter)> options;

    auto setOption(auto&& key, auto&& value);

    AudioInputBuilder();
    ~AudioInputBuilder();
  };

  
  
}
