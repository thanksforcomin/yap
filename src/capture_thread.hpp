#pragma once

#include "audio_input.hpp"
#include "processing.hpp"

#include <functional>

namespace audio {
  class InputWorker {
    std::thread worker_thread;
    std::function<void(audio::FrameWrapper)> subscriber;

    audio::AudioInput audio_input;
    audio::Decoder decoder;
    audio::Resampler resampler;

  public:
    static auto init(auto&& encoder_context) noexcept -> Result<InputWorker>;
    InputWorker(auto&& audio_input, auto&& decoder, auto&& resampler) noexcept;

    InputWorker(const InputWorker &other) = delete;
    auto operator=(const InputWorker &other) -> InputWorker & = delete;

    InputWorker(InputWorker &&other) noexcept = default;
    auto operator=(InputWorker &&other) noexcept -> InputWorker & = default;

    ~InputWorker();

    auto subscribe(auto &&f) -> void;
    auto run() -> void;

  private:
    auto worker_function() -> void;
  };
  
}

#include "src/capture_thread.ipp"
