#include "audio_input.hpp"
#include "src/utils.hpp"
#include <libavcodec/packet.h>

namespace audio {
  PacketWrapper::PacketWrapper() : pack(av_packet_alloc()) {
    if (!pack)
      utils::report_error("Somethign went wrong when creating a packet");
    
    
    pack->data = nullptr;
    pack->size = 0;
  }

  PacketWrapper::~PacketWrapper() { av_packet_unref(pack); }
}

