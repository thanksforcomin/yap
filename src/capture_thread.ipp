#pragma once

#include "capture_thread.hpp"

namespace audio {
  inline auto InputWorker::init(auto&& encoder_context) noexcept -> Result<InputWorker> {
    auto audio_input_ = audio::AudioInputBuilder()
      .setInputFormat("pulse")
      .setDeviceUrl("default")
      .build();
    
    
    if (!audio_input_) {
      utils::report_error("Something went wrong when creating audio device");
      return std::unexpected(audio_input_.error());
    }

    auto audio_input = std::move(*audio_input_);

    
    auto decoder_ = audio::Decoder::init(audio_input.getCodecParams());
    if (!decoder_) {
      utils::report_error("Something went wrong when creating decoder");
      return std::unexpected(decoder_.error());
    }
    auto decoder = std::move(*decoder_);

    auto resampler_ =
        audio::Resampler::init(decoder.getDecoderCtx(), encoder_context);
    if (!resampler_) {
      utils::report_error("Something went wrong when creating resampler");
      return std::unexpected(resampler_.error());
    }
    auto resampler = std::move(*resampler_);

    return InputWorker(std::move(audio_input), std::move(decoder), std::move(resampler));
  }

  InputWorker::InputWorker(auto &&audio_input, auto &&decoder,
                           auto &&resampler) noexcept
    : audio_input(std::forward(audio_input)), decoder(std::forward(decoder)),
      resampler(std::forward(resampler)) {}

  
}
