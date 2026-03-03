#pragma once

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
}

#include "src/utils.hpp"
#include <expected>
#include <memory>
#include <mutex>
#include <tuple>
#include <unordered_map>

namespace proc {
  struct AudioData;

  template <typename T>
  concept Subscriber = requires(T t, AudioData data) {
    { t.process(data) } -> std::same_as<void>;
  };

  template <Subscriber... Subscribers> class Codec {
    auto setup() -> void;
    
  }

  
  
}

#include "processing.ipp"
