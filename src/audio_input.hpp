#pragma once

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavcodec/codec_par.h>
}

#include "src/utils.hpp"
#include "src/errors.hpp"
#include <expected>
#include <memory>
#include <mutex>
#include <tuple>
#include <unordered_map>

namespace audio {

  using namespace errors;
  
  struct PacketWrapper {
    AVPacket *pack;

    PacketWrapper() noexcept;

    PacketWrapper(const PacketWrapper &other) = delete;
    auto operator=(const PacketWrapper &other) -> PacketWrapper & = delete;

    PacketWrapper(PacketWrapper &&other) noexcept ; 
    auto operator=(PacketWrapper &&other) noexcept -> PacketWrapper & ;
    
    ~PacketWrapper();
  };

  inline constexpr auto _formatContextDeleter = [](AVFormatContext *ctx) {
    if (ctx)
      avformat_close_input(&ctx);
  };
  
  class AudioInput {
    
    int32_t audio_index;
    std::unique_ptr<AVFormatContext, decltype(_formatContextDeleter)>
        fmt_ctx_ptr;

  public:
    static auto init(auto &&options, auto &&device_url, auto &&input_format_name) -> Result<AudioInput>;
    
    AudioInput(auto&& format_context, auto&& audio_index);

    AudioInput(const AudioInput &) = delete;
    auto operator=(const AudioInput &) -> AudioInput & = delete;

    AudioInput(AudioInput &&other) = default;
    auto operator=(AudioInput &&other) -> AudioInput & = default;

    ~AudioInput() = default;

    auto process() -> Result<PacketWrapper>;

    auto getCodecParams() const -> AVCodecParameters*;

  private:
    static auto makeInput(AVDictionary *options, utils::formattable auto &&device_url,
                   utils::formattable auto &&input_format_name)
        -> Result<AVFormatContext *>;

    static auto getAudioStream(auto&& format_ctx) -> Result<int>;
  };

  inline constexpr auto _avDictionaryDeleter = [](AVDictionary *dict) {
    if (dict)
      av_dict_free(&dict);
  };
  
  // NOLINTNEXTLINE
  struct AudioInputBuilder {
    std::unique_ptr<AVDictionary, decltype(_avDictionaryDeleter)> options;
    std::string_view device_url;
    std::string_view input_format;

    AudioInputBuilder() = default;
    ~AudioInputBuilder() = default;

    auto setOption(auto &&key, auto &&value) -> AudioInputBuilder &;
    auto setDeviceUrl(auto &&value) -> AudioInputBuilder &;
    auto setInputFormat(auto&& value) -> AudioInputBuilder &;
    auto build() -> Result<AudioInput>;
  };
}

#include "audio_input.ipp"
