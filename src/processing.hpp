#pragma once

#include "src/audio_input.hpp"
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

  template <Subscriber... Subscribers> class OpusCodec {
    
  public:
    OpusCodec() = default;

    OpusCodec(const OpusCodec &other) = delete;
    auto operator=(const OpusCodec &other) -> OpusCodec & = delete;

    OpusCodec(OpusCodec &&other) = default;
    auto operator=(OpusCodec &&other) -> OpusCodec & = default;

    ~OpusCodec() = default;
    
    auto process(const audio::PacketWrapper &packet) -> void;
    auto updateAudioInfo(const audio::AudioInputInfo &info) -> void;

    auto setup(const audio::AudioInputInfo &info) -> void;
  };
}

#include "processing.ipp"
