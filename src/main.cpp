#include <cerrno>
#include <concepts>
#include <cstddef>
#include <cstdio>
#include <format>
#include <iostream>
#include <print>
#include <type_traits>

#include "audio_input.hpp"
#include "processing.hpp"
#include "src/utils.hpp"

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
}

struct PrimitiveLogger {
  void process(const audio::PacketWrapper &wrappa) {
    std::println("Received audio packet of size {}", wrappa.pack.size);
  }

  PrimitiveLogger() = default;

  PrimitiveLogger(const PrimitiveLogger &other) = delete;
  PrimitiveLogger(PrimitiveLogger &&other) = delete;
  auto operator=(const PrimitiveLogger &other) -> PrimitiveLogger& = delete;
  auto operator=(PrimitiveLogger &&other) -> PrimitiveLogger & = delete;

  ~PrimitiveLogger() = default;
};

int main() {
  PrimitiveLogger logger;
  
  avdevice_register_all();
  avformat_network_init();

  auto audio_input_ = audio::AudioInputBuilder(logger)
                          .setInputFormat("pulse")
                          .setDeviceUrl("default")
                          .build();

  if (!audio_input_) {
    utils::report_error("Something went wrong when creating audio device");
    return 1;
  }

  auto audio_input = std::move(*audio_input_);

  audio_input.run();
};
 
