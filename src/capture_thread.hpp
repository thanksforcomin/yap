#pragma once

#include "audio_input.hpp"
#include "processing.hpp"

#include <functional>

namespace audio {
  class InputWorker {
    std::thread worker_thread;
    std::function<void(audio::FrameWrapper)> subscriber;
    std::atomic<bool> is_running;

    audio::AudioInput audio_input;
    audio::Decoder decoder;
    audio::Resampler resampler;

  public:
    static auto init(auto&& encoder_context) noexcept -> Result<InputWorker>;
    InputWorker(auto&& audio_input, auto&& decoder, auto&& resampler) noexcept;

    InputWorker(const InputWorker &other) = delete;
    auto operator=(const InputWorker &other) -> InputWorker & = delete;

    InputWorker(InputWorker &&other) noexcept;
    auto operator=(InputWorker &&other) noexcept -> InputWorker &;

    ~InputWorker();

    auto subscribe(auto &&f) -> Result<void>;

    template <typename T, typename F>
    auto subscribe(&T::F&& f, T& t);
    
    auto run() -> Result<void>;

  private:
    auto worker_function() -> void;
  };
  
}

#include "src/capture_thread.ipp"
