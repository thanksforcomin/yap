#pragma once

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

#include "processing.hpp"

namespace proc {

template <Subscriber... Subscribers>
auto OpusEncoder<Subscribers...>::init(AVCodecParameters *input_params,
                                       Subscribers &...subs)
    -> Result<OpusEncoder> {
    auto decoder = pickCodec(input_params->codec_id);

    if (!decoder)
      return std::unexpected(decoder.error());

    auto decoder_context = setUpDecoder(input_params, *decoder);

    if (!decoder_context)
      return std::unexpected(decoder_context.error());

    auto encoder = pickCodec(AV_CODEC_ID_OPUS);

    if (!encoder)
      return std::unexpected(encoder.error());

    auto encoder_context = setUpEncoder(input_params, *encoder);

    if (!encoder_context)
      return std::unexpected(encoder_context.error());

    auto resampler = setUpResampler(*decoder_context, *encoder_context);

    if (!resampler)
      return std::unexpected(resampler.error());

    return OpusEncoder(*encoder, *encoder_context, *decoder, *decoder_context,
                       *resampler, subs...);
  }

  template <Subscriber... Subscribers>
  OpusEncoder<Subscribers...>::OpusEncoder(auto &&encoder_codec,
                                        auto &&encoder_context,
                                        auto &&decoder_codec,
                                        auto &&decoder_context,
                                        auto &&resampler, Subscribers &...subs)
      : decoder_ptr(decoder_codec), decoder_ctx(decoder_context),
        encoder_ptr(encoder_codec), encoder_ctx(encoder_context),
        swr_context(resampler), subs(std::tie(subs...)) {}
    
  template <Subscriber... Subscribers>
  auto OpusEncoder<Subscribers...>::pickCodec(AVCodecID id)
    -> Result<AVCodec *> {
    AVCodec *codec =
        const_cast<AVCodec *>(avcodec_find_encoder(id));

    if (!codec) {
      utils::report_error("Could not find opus encoder");
      return std::unexpected(EncoderNotFound);
    }

    return codec;
  }

  template <Subscriber... Subscribers>
  auto OpusEncoder<Subscribers...>::setUpEncoder(AVCodecParameters *params, AVCodec *codec)
      -> Result<AVCodecContext *> {
    AVCodecContext *enc_ctx = avcodec_alloc_context3(codec);

    if (!enc_ctx) {
      utils::report_error("Could not allocate encoder context");
      return std::unexpected(EncoderNotAllocated);
    }

    enc_ctx->sample_rate = 48000;
    enc_ctx->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO;
    enc_ctx->sample_fmt = AV_SAMPLE_FMT_FLT;
    enc_ctx->bit_rate = 64000;
    enc_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    if (auto ret = avcodec_open2(enc_ctx, codec, nullptr); ret < 0) {
      utils::report_error("Could not open encoder ");

      avcodec_free_context(&enc_ctx);
      
      return std::unexpected(EncoderNotOpen);
    }

    return enc_ctx;
  }

  template <Subscriber... Subscribers>
  auto OpusEncoder<Subscribers...>::setUpDecoder(AVCodecParameters *params,
                                                 AVCodec *codec)
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

    if (auto ret = avcodec_open2(dec_ctx, codec, nullptr); ret < 0) {
      utils::report_error("Could not open decoder");

      avcodec_free_context(&dec_ctx);

      return std::unexpected(DecoderNotOpen);
    }

    return dec_ctx;
  }

  template <Subscriber... Subscribers>
  auto OpusEncoder<Subscribers...>::setUpResampler(AVCodecContext *decoder,
                                                   AVCodecContext *encoder)
      -> Result<SwrContext *> {
    SwrContext *swr = swr_alloc();

    if (!swr) {
      utils::report_error("Could not allocate resampler context");
      return std::unexpected(ResamplerNotAllocated);
    }

    av_opt_set_chlayout(swr, "in_chlayout", &decoder->ch_layout, 0);
    av_opt_set_chlayout(swr, "out_chlayout", &encoder->ch_layout, 0);
    
    av_opt_set_int(swr, "in_sample_rate", decoder->sample_rate, 0);
    av_opt_set_int(swr, "out_sample_rate", encoder->sample_rate, 0);
    
    av_opt_set_sample_fmt(swr, "in_sample_fmt", decoder->sample_fmt, 0);
    av_opt_set_sample_fmt(swr, "out_sample_fmt", encoder->sample_fmt, 0);

    if (auto ret = swr_init(swr); ret < 0) {
      utils::report_error("Could not initialize resampler context");

      if (swr)
        swr_free(&swr);
      
      return std::unexpected(ResamplerNotAllocated);
    }

    return swr;
  }

  template <Subscriber... Subscribers>
  auto OpusEncoder<Subscribers...>::process(const audio::PacketWrapper &packet)
      -> void {
    std::println("Received a packet");
  }
  
}

