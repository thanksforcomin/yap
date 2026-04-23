
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

