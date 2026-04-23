
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
  auto Decoder::pickDecoder(AVCodecID id) -> Result<AVCodec *> {
    AVCodec *codec = const_cast<AVCodec *>(avcodec_find_decoder(id));
    if (!codec) {
      utils::report_error("Could not find decoder");
      return std::unexpected(EncoderNotFound);
    }
    return codec;
  }
  
  auto Decoder::setUpDecoder(AVCodecParameters *params, AVCodec *codec)
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
  
  auto Decoder::init(AVCodecParameters *input_params) -> Result<Decoder> {
    auto decoder = pickDecoder(input_params->codec_id);
    if (!decoder)
      return std::unexpected(decoder.error());
    
    auto decoder_context = setUpDecoder(input_params, *decoder);
    if (!decoder_context)
      return std::unexpected(decoder_context.error());
    
    return Decoder(*decoder, *decoder_context);
  }
  
  Decoder::Decoder(auto &&dec_ptr, auto &&dec_ctx)
    : decoder_ptr(dec_ptr), decoder_ctx(dec_ctx) {}
  
  auto Decoder::getDecoderCtx() const -> const AVCodecContext & {
    return *decoder_ctx;
  }
  
  auto Decoder::process(auto &&data) {
    // Implementation that calls next->process(...) when ready
  }
} // namespace proc