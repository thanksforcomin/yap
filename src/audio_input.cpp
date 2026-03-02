#include "audio_input.hpp"

namespace audio {
  PacketWrapper::PacketWrapper() : pack() {
    pack.data = nullptr;
    pack.size = 0;
  }

  PacketWrapper::~PacketWrapper() { av_packet_unref(&pack); }

  template <AudioSubscriber... Subscribers>
  AudioInput::AudioInput(Subscribers &&...subscribers)
}
