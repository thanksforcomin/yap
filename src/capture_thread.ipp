#pragma once

#include "capture_thread.hpp"
#include "src/audio_input.hpp"

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

  inline auto InputWorker::subscribe(auto &&f) -> Result<void> {
    subscriber = f;

    return {};
  }

  InputWorker::InputWorker(InputWorker &&other) noexcept
      : worker_thread(std::move(other.worker_thread)),
        subscriber(std::move(other.subscriber)),
        is_running(other.is_running.load()),
        audio_input(std::move(other.audio_input)),
        decoder(std::move(other.decoder)),
        resampler(std::move(other.resampler)) {}

  auto InputWorker::operator=(InputWorker &&other) noexcept -> InputWorker & {
    if (this == &other)
      return *this;

    if (is_running.load()) {
      is_running.store(false);

      if (worker_thread.joinable())
        worker_thread.join();
    }

    
    worker_thread = std::move(other.worker_thread);
    subscriber    = std::move(other.subscriber);
    is_running.store(other.is_running.load());
    audio_input   = std::move(other.audio_input);
    decoder       = std::move(other.decoder);
    resampler     = std::move(other.resampler);
    
    other.is_running.store(false);
    
    return *this;
  }

  InputWorker::~InputWorker() {
    if (is_running.load()) {
      if (worker_thread.joinable())
        worker_thread.join();
    }
  }

  inline auto InputWorker::run() -> Result<void> {
    worker_thread = std::thread(&InputWorker::worker_function, this);
    
    return {};
  }
}
