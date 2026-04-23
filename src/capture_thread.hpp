#pragma once

#include "audio_input.hpp"
#include "processing.hpp"

#include <functional>

namespace audio {
  class InputWorker {
    std::thread worker_thread;
    std::function<void(proc::FrameWrapper)> subscriber;

  public:
    static auto init() noexcept -> Result<InputWorker>;
    InputWorker() noexcept;

    InputWorker(const InputWorker &other) = delete;
    auto operator=(const InputWorker &other) -> InputWorker & = delete;

    InputWorker(InputWorker &&other) noexcept;
    auto operator=(InputWorker &&other) noexcept -> InputWorker &;

    ~InputWorker();
  };
  
}
