#pragma once

#include <libavutil/frame.h>
#include <print>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/codec_id.h>
#include <libavcodec/codec_par.h>
#include <libavcodec/codec.h>
#include <libavutil/opt.h>
#include <libavutil/rational.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libavcodec/defs.h>
#include <libavutil/channel_layout.h>
}

#include "src/audio_input.hpp"
#include "src/utils.hpp"
#include <memory>
#include <iostream>
#include "processing.hpp"

namespace proc {
  template <Subscriber Next>
  auto Decoder<Next>::pickDecoder(AVCodecID id) -> Result<AVCodec *> {
    AVCodec *codec =
      const_cast<AVCodec *>(avcodec_find_decoder(id));
    
    if (!codec) {
      utils::report_error("Could not find opus encoder");
      return std::unexpected(EncoderNotFound);
    }

    return codec;
  }

  template <Subscriber Next>
  auto Decoder<Next>::setUpDecoder(AVCodecParameters *params, AVCodec *codec)
      -> Result<AVCodecContext *> {
    AVCodecContext *dec_ctx = avcodec_alloc_context3(codec);
    
    if (!dec_ctx) {
      utils::report_error("Could not allocate decoder context");
      return std::unexpected(DecoderNotAllocated);
    }
    
    if (auto ret = avcodec_parameters_to_context(dec_ctx, params); ret < 0) {
      utils::report_error("Could not set decoder context parameters");
      
      avcodec_free_context(&dec_ctx);
      
      return std::unexpected(DecoderNotOpen);
    }
    
    // JUST TO BE SURE
    dec_ctx->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO;
    
    if (auto ret = avcodec_open2(dec_ctx, codec, nullptr); ret < 0) {
      utils::report_error("Could not open decoder");
      
      avcodec_free_context(&dec_ctx);
      
      return std::unexpected(DecoderNotOpen);
    }

    return dec_ctx;
  }

  template <Subscriber Next>
  auto Decoder<Next>::init(AVCodecParameters *input_params)
      -> Result<Decoder> {
    auto decoder = pickDecoder(input_params->codec_id);

    if (!decoder)
      return std::unexpected(decoder.error());

    auto decoder_context = setUpDecoder(input_params, *decoder);
    if (!decoder_context)
      return std::unexpected(decoder_context.error());

    return Decoder(*decoder, *decoder_context);
  }

  template <Subscriber Next>
  Decoder<Next>::Decoder(auto &&dec_ptr, auto &&dec_ctx)
      : decoder_ptr(dec_ptr), decoder_ctx(dec_ctx) {}

  template <Subscriber Next>
  auto Decoder<Next>::getDecoderPtr() const -> const AVCodecContext & {
    return *decoder_ctx;
  }

  template <Subscriber Next>
  auto Decoder<Next>::setNext(const Next &obj) -> void {
    next = &obj;
  }

  template <Subscriber Next> auto Decoder<Next>::process(auto &&data) -> void {}

} // namespace proc

namespace proc {
  template <Subscriber Next>
  auto Encoder<Next>::pickEncoder(AVCodecID id) -> Result<AVCodec *> {
    AVCodec *codec =
      const_cast<AVCodec *>(avcodec_find_encoder(id));
    
    if (!codec) {
      utils::report_error("Could not find opus encoder");
      return std::unexpected(EncoderNotFound);
    }
    
    return codec;
  }

  template <Subscriber Next>
  auto Encoder<Next>::setUpEncoder(AVCodecParameters *params, AVCodec *codec,
                                   int sample_rate, AVChannelLayout ch_layout,
                                   AVSampleFormat format, int bit_rate,
                                   int std_compliance)
      -> Result<AVCodecContext *> {
    AVCodecContext *enc_ctx = avcodec_alloc_context3(codec);

    if (!enc_ctx) {
      utils::report_error("Could not allocate encoder context");
      return std::unexpected(EncoderNotAllocated);
    }

    enc_ctx->sample_rate = sample_rate;
    enc_ctx->ch_layout = ch_layout;
    enc_ctx->sample_fmt = format;
    enc_ctx->bit_rate = bit_rate;
    enc_ctx->strict_std_compliance = std_compliance;

    if (auto ret = avcodec_open2(enc_ctx, codec, nullptr); ret < 0) {
      utils::report_error("Could not open encoder ");

      avcodec_free_context(&enc_ctx);
      
      return std::unexpected(EncoderNotOpen);
    }

    return enc_ctx;
    
  }
}


