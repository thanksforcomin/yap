#include <cerrno>
#include <concepts>
#include <cstddef>
#include <format>
#include <iostream>
#include <print>
#include <type_traits>

#include "audio_input.ipp"

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

  const char* device_url = "default";
  const char* input_format_name = "pulse";

  auto audio_input = audio::AudioInputBuilder()
                         .setInputFormat("pulse")
                         .setDeviceUrl("default")
                         .build();

  audio_input.run();
};
 
