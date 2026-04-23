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

namespace audio {
  auto Encoder::pickEncoder(AVCodecID id) -> Result<AVCodec *> {
    AVCodec *codec = const_cast<AVCodec *>(avcodec_find_encoder(id));
    if (!codec) {
      utils::report_error("Could not find encoder");
      return std::unexpected(EncoderNotFound);
    }
    return codec;
  }
  
  auto Encoder::setUpEncoder(AVCodec *codec, int sample_rate,
                             AVChannelLayout ch_layout, AVSampleFormat format,
                             int bit_rate, int std_compliance)
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
      utils::report_error("Could not open encoder");
      avcodec_free_context(&enc_ctx);
      return std::unexpected(EncoderNotOpen);
    }
    return enc_ctx;
  }
  
  auto Encoder::init(AVCodecID id, int sample_rate,
                     AVChannelLayout ch_layout, AVSampleFormat format,
                     int bit_rate, int std_compliance)
    -> Result<Encoder> {
    auto encoder = pickEncoder(id);
    if (!encoder)
      return std::unexpected(encoder.error());
    
    auto encoder_context = setUpEncoder(*encoder, sample_rate, ch_layout,
                                        format, bit_rate, std_compliance);
    if (!encoder_context)
      return std::unexpected(encoder_context.error());
    
    return Encoder(std::move(*encoder), std::move(*encoder_context));
  }
  
  Encoder::Encoder(auto &&enc_ptr, auto &&enc_ctx)
    : encoder_ptr(enc_ptr), encoder_ctx(enc_ctx) {}
  
  auto Encoder::getEncoderCtx() const -> const AVCodecContext & {
    return *encoder_ctx;
  }
  
  Encoder::~Encoder() {
  }
} // namespace proc
