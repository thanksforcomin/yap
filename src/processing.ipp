#pragma once

#include "src/audio_input.hpp"
#include <libavcodec/codec.h>
extern "C" {
#include <libavcodec/codec_par.h>
}

#include "processing.hpp"

namespace proc {
  template <Subscriber... Subscribers>
  auto OpusCodec<Subscribers...>::updateAudioInfo(
                                                  const audio::AudioInputInfo &info) -> void {
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_OPUS)
  }
  
}

