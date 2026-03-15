#pragma once

#include "src/audio_input.hpp"

extern "C" {
#include <libavcodec/codec_par.h>
#include <libavutil/frame.h>
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

  enum ProcessingError {
    ResamplingError,
    EncoderNotFound,
    EncoderNotAllocated,
    EncoderNotOpen,
    DecoderNotAllocated,
    DecoderNotOpen,
    ResamplerNotAllocated
  };
  
  template <typename T>
  using Result = std::expected<T, ProcessingError>;
  
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

  inline constexpr auto _frameDeleter = [](AVFrame *frame) {
    if (frame)

      av_frame_free(&frame);
  };

  struct Resampler {
    std::unique_ptr<SwrContext, decltype(_resamplerDeleter)> swr_context;
    std::unique_ptr<AVFrame, decltype(_frameDeleter)> frame;
  };

  template <Subscriber... Subscribers> class OpusEncoder {
    std::unique_ptr<AVCodec> decoder_ptr;
    std::unique_ptr<AVCodecContext, decltype(_codecContextDeleter)> decoder_ctx;
    std::unique_ptr<AVCodec> encoder_ptr;
    std::unique_ptr<AVCodecContext, decltype(_codecContextDeleter)> encoder_ctx;
    std::optional<Resampler> resampler;
    std::tuple<Subscribers&...> subs;

  public:
    static auto init(AVCodecParameters *input_params, Subscribers &...subs)
        -> Result<OpusEncoder>;
    

    OpusEncoder() = default;
    OpusEncoder(auto &&encoder_codec, auto &&encoder_context,
                auto &&decoder_codec, auto &&decoder_context, auto &&resampler,
                Subscribers &...subs);

    OpusEncoder(const OpusEncoder &other) = delete;
    auto operator=(const OpusEncoder &other) -> OpusEncoder & = delete;

    OpusEncoder(OpusEncoder &&other) = default;
    auto operator=(OpusEncoder &&other) -> OpusEncoder & = default;

    ~OpusEncoder() = default;

    auto process(const audio::PacketWrapper &packet) -> void;

  private:
    static auto pickCodec(AVCodecID id) -> Result<AVCodec *>;
    static auto setUpEncoder(AVCodecParameters *params, AVCodec *codec)
        -> Result<AVCodecContext *>;
    static auto setUpResampler(AVCodecContext *decoder, AVCodecContext *encoder)
        -> Result<SwrContext *>;
    static auto setUpDecoder(AVCodecParameters *params, AVCodec *codec)
        -> Result<AVCodecContext *>;
  };
}

#include "processing.ipp"
