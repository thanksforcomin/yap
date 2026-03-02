#include <cerrno>
#include <concepts>
#include <cstddef>
#include <format>
#include <iostream>
#include <print>
#include <type_traits>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
}

namespace utils {
template <typename T>
concept integral = std::integral<std::remove_reference_t<T>>;

  template <typename T>
  using element_type = std::decay_t<decltype(std::declval<std::decay_t<T>>()[0])>;

  template <typename T>
  concept formattable = std::formattable<T, element_type<T>>;
}

void report_error(utils::integral auto&& code, utils::formattable auto&& message) {
  static std::array<char, 128> errbuf;
  av_strerror(code, errbuf.data(), sizeof(errbuf));
  std::println("{}: {}", message, errbuf.data());
}

void report_error(utils::formattable auto &&message) {
  std::print("{}", message);
}

int main() {
  avdevice_register_all();
  avformat_network_init();

  const char* device_url = "default";
  const char* input_format_name = "pulse";

  AVInputFormat *input_format =
    const_cast<AVInputFormat *>(av_find_input_format(input_format_name));
  
  if (!input_format) {
    std::print("Unknown input format: {}", input_format_name);
    return 1;
  }

  AVFormatContext *fmt_context = nullptr;
  AVDictionary *options = nullptr;

  if (auto ret = avformat_open_input(&fmt_context, device_url,
                                     input_format, &options);
      ret < 0) {
    report_error(ret, "Failed to open device");
    return 1;
  }

  std::println("Help");

  if (auto ret = avformat_find_stream_info(fmt_context, nullptr); ret < 0) {
    std::print("No audio stream found");
    avformat_close_input(&fmt_context);
    return 1;
  }

  std::println("Seaching for audio stream...");

  int audio_stream_index = -1;
  for (int i = 0; i < fmt_context->nb_streams; ++i) {
    std::println("{}", i);
    if (fmt_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      audio_stream_index = i;
      break;
    }
  }

  if (audio_stream_index == -1) {
    report_error("No audio stream found");
    return 1;
  }

  AVPacket packet;
  packet.data = nullptr;
  packet.size = 0;

  std::println("Capturing audio...");

  while (true) {
    if (auto ret = av_read_frame(fmt_context, &packet); ret < 0) {
      if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
        break;
      report_error("Error reading frame:");
    }

    if (packet.stream_index == audio_stream_index) {
      static int count = 0;
      if (++count % 100 == 0)
        std::println("Got audio packet, size {}", packet.size);
    }

    av_packet_unref(&packet);
  }

  avformat_close_input(&fmt_context);
  return 0;
};
 
