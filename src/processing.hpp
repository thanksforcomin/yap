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

namespace proc {

  enum ProcessingError {
    ResamplingError,
    EncoderNotFound,
    EncoderNotAllocated,
    EncoderNotOpen,
    DecoderNotAllocated,
    DecoderNotOpen,
    ResamplerNotAllocated,
    FrameAllocationFault,
    AudioArrayError
  };
  
  template <typename T>
  using Result = std::expected<T, ProcessingError>;

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

  template<Subscriber Next>
  class Decoder {
    std::unique_ptr<AVCodec> decoder_ptr;
    std::unique_ptr<AVCodecContext, decltype(_codecContextDeleter)> decoder_ctx;

    Next *next;

  public:
    static auto init(AVCodecParameters *input_params) -> Result<Decoder>;

    Decoder() = default;
    Decoder(auto &&dec_ptr, auto &&dec_ctx);

    Decoder(const Decoder &) = delete;
    auto operator=(const Decoder &) -> Decoder & = delete;

    Decoder(Decoder &&) = default;
    auto operator=(Decoder &&) -> Decoder & = default;

    ~Decoder() = default;

    auto getDecoderPtr() const -> const AVCodecContext &;

    auto setNext(const Next &obj) -> void;
    auto process(auto&& data) -> void;

  private:
    static auto pickDecoder(AVCodecID id) -> Result<AVCodec *>;
    static auto setUpDecoder(AVCodecParameters *params, AVCodec *codec)
        -> Result<AVCodecContext *>;
  };


  template <Subscriber Next>
  class Resampler {
    std::unique_ptr<SwrContext, decltype(_resamplerDeleter)> swr_context;

    Next *next;

  public:
    static auto init(const AVCodecContext &decoder_context,
                     const AVCodecContext &encoder_context)
        -> Result<Resampler>;

    Resampler() = default;
    Resampler(auto &&swr_context);

    Resampler(const Resampler &) = delete;
    auto operator=(const Resampler &) -> Resampler & = delete;

    Resampler(Resampler &&) = default;
    auto operator=(Resampler &&) -> Resampler & = default;

    ~Resampler() = default;

    auto setNext(const Next &obj) -> void;
    auto process(auto&& data) -> void;
  };
  
  template <Subscriber Next>
  class Encoder {
    std::thread worker_thread;
    std::mutex mutex;
    std::atomic<bool> is_running;
    std::condition_variable has_data;

    std::unique_ptr<AVCodec> encoder_ptr;
    std::unique_ptr<AVCodecContext, decltype(_codecContextDeleter)> encoder_ctx;
    std::unique_ptr<AVAudioFifo, decltype(_fifoDeleter)> fifo_buffer;

  public:
    static auto init(AVCodecID id, int sample_rate, AVChannelLayout ch_layout,
                     AVSampleFormat format, int bit_rate, int std_compliance)
        -> Result<Encoder>;

    Encoder() = default;
    Encoder(auto &&enc_ptr, auto &&enc_ctx, auto &&worker_thread);

    Encoder(const Encoder &) = delete;
    auto operator=(const Encoder &) -> Encoder & = delete;

    Encoder(Encoder &&) = default;
    auto operator=(Encoder &&) -> Encoder & = default;

    ~Encoder();
    
    auto start() -> void;
    auto setNext(const Next &next) -> void;
    auto process(auto &&data) -> void;

  private:
    static auto pickEncoder(AVCodecID id) -> Result<AVCodec *>;
    static auto setUpEncoder(AVCodecParameters *params, AVCodec *codec,
                             int sample_rate, AVChannelLayout ch_layout,
                             AVSampleFormat format, int bit_rate,
                             int std_compliance) -> Result<AVCodecContext *>;
    
  };

}

#include "processing.ipp"
