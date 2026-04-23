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

namespace audio {
  auto Resampler::setUpResampler(const AVCodecContext *decoder,
                                 const AVCodecContext *encoder)
    -> Result<SwrContext *> {
    SwrContext *swr = swr_alloc();
    if (!swr) {
      utils::report_error("Could not allocate resampler context");
      return std::unexpected(ResamplerNotAllocated);
    }
    int ret = swr_alloc_set_opts2(&swr,
                                  &encoder->ch_layout, encoder->sample_fmt, encoder->sample_rate,
                                  &decoder->ch_layout, decoder->sample_fmt, decoder->sample_rate,
                                  0, nullptr);
    if (ret < 0) {
      swr_free(&swr);
      utils::report_error("Could not set resampler options");
      return std::unexpected(ResamplerNotAllocated);
    }
    if (auto ret = swr_init(swr); ret < 0) {
      utils::report_error("Could not initialize resampler context");
      if (swr)
        swr_free(&swr);
      return std::unexpected(ResamplerNotAllocated);
    }
    return swr;
  }
  
  auto Resampler::setUpOutFrame(const AVCodecContext *encoder, int max_samples)
    -> Result<AVFrame *> {
    auto out_frame = av_frame_alloc();
    out_frame->format = encoder->sample_fmt;
    out_frame->ch_layout = encoder->ch_layout;
    out_frame->sample_rate = encoder->sample_rate;
    out_frame->nb_samples = max_samples;
    
    if (auto ret = av_frame_get_buffer(out_frame, 0); ret < 0) {
      utils::report_error(ret, "Issues with frame buffer allocation");
      av_frame_unref(out_frame);
      return std::unexpected(FrameAllocationFault);
    }
    return out_frame;
  }
  
  auto Resampler::init(const AVCodecContext &decoder_context,
                       const AVCodecContext &encoder_context)
    -> Result<Resampler> {
    auto resampler = setUpResampler(&decoder_context, &encoder_context);
    if (!resampler)
      return std::unexpected(ResamplerNotAllocated);
    
    auto frame = setUpOutFrame(&encoder_context, 960);
    if (!frame)
      return std::unexpected(ResamplerNotAllocated);
    
    return Resampler(*resampler, *frame);
}
  
  Resampler::Resampler(auto &&swr_context, auto &&frame_out)
    : swr_context(swr_context), out_frame(frame_out) {}
  
  auto Resampler::process(auto &&data) -> void {
    // Implementation that calls next->process(...)
  }
}


