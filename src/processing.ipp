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
}

#include "src/audio_input.hpp"
#include "src/utils.hpp"
#include <memory>

#include "processing.hpp"

namespace proc {
  template <Subscriber... Subscribers>
  auto OpusEncoder<Subscribers...>::pickCodec()
    -> Result<AVCodec *> {
    AVCodec *codec =
        const_cast<AVCodec *>(avcodec_find_encoder(AV_CODEC_ID_OPUS));

    if (!codec) {
      utils::report_error("Could not find opus encoder");
      return std::unexpected(EncoderNotFound);
    }

    return codec;
  }

  template <Subscriber... Subscribers>
  auto OpusEncoder<Subscribers...>::allocateCodec(AVCodecParameters *params,
                                                  AVCodec *codec)
      -> Result<AVCodecContext *> {
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);

    if (!codec_ctx) {
      utils::report_error("Could not allocate opus encoder");
      return std::unexpected(EncoderNotAllocated);
    }

    int target_sample_rate = params->sample_rate;
    if (target_sample_rate != 48000 && target_sample_rate != 24000 &&
        target_sample_rate != 16000 && target_sample_rate != 12000 &&
        target_sample_rate != 8000) {
      target_sample_rate = 48000;
    }

    codec_ctx->sample_rate = target_sample_rate;
    codec_ctx->ch_layout = params->ch_layout;
    codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    codec_ctx->bit_rate = 64000;
    codec_ctx->time_base = av_make_q(1, target_sample_rate);
    codec_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    if (auto ret = avcodec_open2(codec_ctx, codec, nullptr); ret < 0) {
      utils::report_error("Could not open opus encoder");

      avcodec_free_context(&codec_ctx);
      
      return std::unexpected(EncoderNotAllocated);
    }

    return codec_ctx;
  }

  template <Subscriber... Subscribers>
  auto OpusEncoder<Subscribers...>::setUpResampler(AVCodecParameters *params,
                               AVCodecContext *context)
      -> Result<SwrContext *> {
    SwrContext *swr = swr_alloc();

    if (!swr) {
      utils::report_error("Could not allocate resampler context");
      return std::unexpected(ResamplerNotAllocated);
    }

    av_opt_set_chlayout(swr, "in_chlayout", &params->ch_layout, 0);
    av_opt_set_chlayout(swr, "out_chlayout", &context->ch_layout, 0);
    av_opt_set_int(swr, "in_sample_rate", params->sample_rate, 0);
    av_opt_set_int(swr, "out_sample_rate", context->sample_rate, 0);
    av_opt_set_sample_fmt(swr, "in_sample_fmt", static_cast<AVSampleFormat>(params->format), 0);
    av_opt_set_sample_fmt(swr, "out_sample_fmt", context->sample_fmt, 0);

    if (auto ret = swr_init(swr); ret < 0) {
      utils::report_error("Could not initialize resampler context");

      if (swr)
        swr_free(&swr);
      
      return std::unexpected(ResamplerNotAllocated);
    }

    return swr;
  }
  
}

