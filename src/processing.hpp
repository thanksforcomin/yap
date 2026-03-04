#pragma once

#include "src/audio_input.hpp"
extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libswresample/swresample.h>
}

#include "src/utils.hpp"
#include <expected>
#include <memory>
#include <mutex>
#include <tuple>
#include <unordered_map>
 
namespace proc {
  struct AudioData;

  template <typename T>
  concept Subscriber = requires(T t, AudioData data) {
    { t.process(data) } -> std::same_as<void>;
  };

  inline constexpr auto _codecContextDeleter = [](AVCodecContext *context) {
    if(context)
      avcodec_free_context(&context);
  };

  inline constexpr auto _resamplerDeleter = [](SwrContext *context) {
    if(context)
      swr_free(&context);
  };

  template <Subscriber... Subscribers> class OpusEncoder {
    std::unique_ptr<AVCodecContext, decltype(_codecContextDeleter)> codec_ctx;
    std::optional<std::unique_ptr<SwrContext, decltype(_resamplerDeleter)>>
        resampler;
    std::tuple<Subscribers&...> subs;
    
  public:
    OpusEncoder();
    OpusEncoder(AVCodecParameters *input_params);

    OpusEncoder(const OpusEncoder &other) = delete;
    auto operator=(const OpusEncoder &other) -> OpusEncoder & = delete;

    OpusEncoder(OpusEncoder &&other) = default;
    auto operator=(OpusEncoder &&other) -> OpusEncoder & = default;

    ~OpusEncoder() = default;

    auto process(const audio::PacketWrapper &packet) -> void;

  private:
    auto allocateCodec(AVCodecParameters *params) -> AVCodecContext *;
    auto setUpCodec() -> void;

    auto setUpResampler();
  };
}

#include "processing.ipp"
