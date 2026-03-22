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

  template <Subscriber... Subscribers>
  auto OpusEncoder<Subscribers...>::init(AVCodecParameters *input_params,
                                         Subscribers &...subs)
    -> Result<OpusEncoder> {
    auto decoder = pickDecoder(input_params->codec_id);

    if (!decoder)
      return std::unexpected(decoder.error());

    auto decoder_context = setUpDecoder(input_params, *decoder);

    if (!decoder_context)
      return std::unexpected(decoder_context.error());

    auto encoder = pickEncoder(AV_CODEC_ID_OPUS);

    if (!encoder)
      return std::unexpected(encoder.error());

    auto encoder_context = setUpEncoder(input_params, *encoder);

    if (!encoder_context)
      return std::unexpected(encoder_context.error());

    auto resampler = setUpResampler(*decoder_context, *encoder_context);

    if (!resampler)
      return std::unexpected(resampler.error());

    auto in_frame = av_frame_alloc();
    // TODO: fix this magic number bullshit
    auto out_frame = setUpOutFrame(*encoder_context, 200);

    std::println("Set up the Opus encoder");

    return OpusEncoder(*encoder, *encoder_context, *decoder, *decoder_context,
                       *resampler, in_frame, *out_frame, subs...);
    
  }

  template <Subscriber... Subscribers>
  OpusEncoder<Subscribers...>::OpusEncoder(
      auto &&encoder_codec, auto &&encoder_context, auto &&decoder_codec,
      auto &&decoder_context, auto &&resampler, auto &&frame_in,
      auto &&frame_out, Subscribers &...subs)
      : decoder_ptr(decoder_codec), decoder_ctx(decoder_context),
        encoder_ptr(encoder_codec), encoder_ctx(encoder_context),
        swr_context(resampler), frame_in(frame_in), frame_out(frame_out),
        subs(std::tie(subs...)) {}
    
  template <Subscriber... Subscribers>
  auto OpusEncoder<Subscribers...>::pickEncoder(AVCodecID id)
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
  auto OpusEncoder<Subscribers...>::pickDecoder(AVCodecID id)
    -> Result<AVCodec *> {
    AVCodec *codec =
        const_cast<AVCodec *>(avcodec_find_decoder(id));

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

    // JUST TO BE SURE
    dec_ctx->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO;
    
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

  template <Subscriber... Subscribers>
  auto OpusEncoder<Subscribers...>::setUpOutFrame(AVCodecContext *encoder,
                                                  int max_samples)
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

  template <Subscriber... Subscribers>
  auto OpusEncoder<Subscribers...>::process(const audio::PacketWrapper &packet)
      -> void {
    if (auto ret = avcodec_send_packet(decoder_ctx.get(), packet.pack);
        ret < 0) {
      utils::report_error(ret, "Error sending packet to decoder");
    }

    int retcode = 0;
    while (retcode >= 0) {
      retcode = avcodec_receive_frame(decoder_ctx.get(), frame_in.get());
      
      if (retcode == AVERROR(EAGAIN) || retcode == AVERROR_EOF)
        break;

      if (retcode < 0) {
        utils::report_error(retcode, "Error retreiving frame from decoder");
        break;
      }
      
      std::println("Currently awaiting {} samples", frame_out->nb_samples);

      int needed_samples =
          swr_get_out_samples(swr_context.get(), frame_in->nb_samples);

      if(needed_samples > frame_out->nb_samples) {
        av_frame_unref(frame_out.get());
        
        frame_out->nb_samples = needed_samples;
        frame_out->format = encoder_ctx->sample_fmt;
        frame_out->ch_layout = encoder_ctx->ch_layout;
        frame_out->sample_rate = encoder_ctx->sample_rate;

        if (auto ret = av_frame_get_buffer(frame_out.get(), 0); ret < 0) {
          utils::report_error(ret, "Failed to make the buffer bigger");
          av_frame_unref(frame_out.get());
          return;
        } else {
          std::println("Made the buffer bigger, it's {} bytes now", needed_samples);
        }
      }
                                                               
      if (auto ret = swr_convert_frame(swr_context.get(), frame_out.get(),
                                       frame_in.get());
          ret < 0) {
        utils::report_error(ret, "Failed to reformat the frame");
        return;
      }

      if (auto ret = avcodec_send_frame(encoder_ctx.get(), frame_out.get());
          ret < 0) {
        utils::report_error(ret, "Failed to send data to encoder");
        return;
      }
      
    }
  }
  
}

