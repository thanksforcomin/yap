#pragma once

#include "src/audio_input.hpp"
#include <condition_variable>
#include <libavcodec/defs.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>

extern "C" {
#include <libavcodec/codec_par.h>
#include <libavutil/frame.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}

#include "src/utils.hpp"
#include <expected>
#include <memory>
#include <thread>
#include <vector>
#include <mutex>
#include <tuple>
#include <unordered_map>

namespace audio {

  using namespace errors;

  struct FrameWrapper {
    AVFrame *frame;

    FrameWrapper() noexcept;

    FrameWrapper(const FrameWrapper &other) = delete;
    auto operator=(const FrameWrapper &other) -> FrameWrapper & = delete;

    FrameWrapper(FrameWrapper &&other) noexcept;
    auto operator=(FrameWrapper &&other) noexcept -> FrameWrapper &;
    
    ~FrameWrapper();
  };

  struct SimpleCaster {
    template<typename T> constexpr operator T &();

    template <typename T> constexpr operator T &&();
  };
  
  template <typename T>
  concept Subscriber = requires(T t, SimpleCaster data) {
    { t.process(data) } -> std::same_as<void>;
  };

  inline auto _codecContextDeleter = [](AVCodecContext *context) {
    if(context)
      avcodec_free_context(&context);
  };

  inline auto _resamplerDeleter = [](SwrContext *context) {
    if(context)
      swr_free(&context);
  };

  inline auto _frameDeleter = [](AVFrame *frame) {
    if (frame)
      av_frame_free(&frame);
  };

  inline auto _fifoDeleter = [](AVAudioFifo *fifo) {
    if (fifo)
      av_audio_fifo_free(fifo);
  };

  class Decoder {
    std::unique_ptr<AVCodec> decoder_ptr;
    std::unique_ptr<AVCodecContext, decltype(_codecContextDeleter)> decoder_ctx;

  public:
    static auto init(AVCodecParameters *input_params) -> Result<Decoder>;

    Decoder() = default;
    Decoder(auto &&dec_ptr, auto &&dec_ctx);

    Decoder(const Decoder &) = delete;
    auto operator=(const Decoder &) -> Decoder & = delete;

    Decoder(Decoder &&) = default;
    auto operator=(Decoder &&) -> Decoder & = default;

    ~Decoder() = default;

    auto getDecoderCtx() const -> const AVCodecContext &;

    auto process(auto&& data);

  private:
    static auto pickDecoder(AVCodecID id) -> Result<AVCodec *>;
    static auto setUpDecoder(AVCodecParameters *params, AVCodec *codec)
        -> Result<AVCodecContext *>;
  };

  class Resampler {
    std::unique_ptr<SwrContext, decltype(_resamplerDeleter)> swr_context;
    std::unique_ptr<AVFrame, decltype(_frameDeleter)> out_frame;

  public:
    static auto init(const AVCodecContext &decoder_context,
                     const AVCodecContext &encoder_context)
        -> Result<Resampler>;

    Resampler() = default;
    Resampler(auto &&swr_context, auto &&frame_out);

    Resampler(const Resampler &) = delete;
    auto operator=(const Resampler &) -> Resampler & = delete;

    Resampler(Resampler &&) = default;
    auto operator=(Resampler &&) -> Resampler & = default;

    ~Resampler() = default;

    auto process(auto &&data) -> void;

  private:
    static auto setUpResampler(const AVCodecContext *decoder, const AVCodecContext *encoder)
        -> Result<SwrContext *>;
    static auto setUpOutFrame(const AVCodecContext *encoder, int max_samples)
        -> Result<AVFrame *>;
  };
  
  class Encoder {
    std::unique_ptr<AVCodec> encoder_ptr;
    std::unique_ptr<AVCodecContext, decltype(_codecContextDeleter)> encoder_ctx;
    std::unique_ptr<AVAudioFifo, decltype(_fifoDeleter)> fifo_buffer;

  public:
    static auto init(AVCodecID id, int sample_rate, AVChannelLayout ch_layout,
                     AVSampleFormat format, int bit_rate, int std_compliance)
        -> Result<Encoder>;

    Encoder() = default;
    Encoder(auto &&enc_ptr, auto &&enc_ctx);

    Encoder(const Encoder &) = delete;
    auto operator=(const Encoder &) -> Encoder & = delete;

    Encoder(Encoder &&) = default;
    auto operator=(Encoder &&) -> Encoder & = default;

    ~Encoder();

    auto getEncoderCtx() const -> const AVCodecContext &;
    
    auto start() -> void;
    auto process(auto &&data);

  private:
    auto workerThread() -> void;
    
    static auto pickEncoder(AVCodecID id) -> Result<AVCodec *>;
    static auto setUpEncoder(AVCodec *codec, int sample_rate,
                             AVChannelLayout ch_layout, AVSampleFormat format,
                             int bit_rate, int std_compliance)
        -> Result<AVCodecContext *>;
  };
  
}
