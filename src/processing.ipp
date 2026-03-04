#pragma once


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/codec_id.h>
#include <libavcodec/codec_par.h>
#include <libavcodec/codec.h>
}

#include "src/audio_input.hpp"
#include "src/utils.hpp"
#include <memory>

#include "processing.hpp"

namespace proc {
  template <Subscriber... Subscribers>
  OpusEncoder<Subscribers...>::OpusEncoder(AVCodecParameters *input_params)
    : codec_ctx(allocateCodec(input_params)) {
    
  }

  template <Subscriber... Subscribers>
  auto OpusEncoder<Subscribers...>::allocateCodec(AVCodecParameters *params)
      -> AVCodecContext * {
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_OPUS);
    if (!codec) {
      utils::report_error("Failed to create codec");
      return nullptr;
    }

    AVCodecContext *context = avcodec_alloc_context3(codec);
    if (!context) {
      utils::report_error("Failed to allocate codec");
      return nullptr;
    }

    return context;
  }

  template <Subscriber... Subscribers>
  auto OpusEncoder<Subscribers...>::setUpCodec() -> void {
    
  }
 
  
}

