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
int main() {
  avdevice_register_all();
  avformat_network_init();

  auto audio_input_ = audio::AudioInputBuilder()
                          .setInputFormat("pulse")
                          .setDeviceUrl("default")
                          .build();
  

  if (!audio_input_) {
    utils::report_error("Something went wrong when creating audio device");
    return 1;
  }
  auto audio_input = std::move(*audio_input_);

  auto decoder_ = audio::Decoder::init(audio_input.getCodecParams());
  if (!decoder_) {
    utils::report_error("Something went wrong when creating decoder");
    return 1;
  }
  auto decoder = std::move(*decoder_);

  
  
};
 
