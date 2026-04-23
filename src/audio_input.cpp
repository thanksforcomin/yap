#include "audio_input.hpp"
#include "src/utils.hpp"
#include <libavcodec/packet.h>

namespace audio {
  PacketWrapper::PacketWrapper() noexcept : pack(av_packet_alloc())  {
    if (!pack)
      utils::report_error("Somethign went wrong when creating a packet");
    
    pack->data = nullptr;
    pack->size = 0;
  }
  
  PacketWrapper::PacketWrapper(PacketWrapper &&other) noexcept
  : pack(other.pack) {
    other.pack = nullptr;
  }
  
  // Move assignment operator
  auto PacketWrapper::operator=(PacketWrapper &&other) noexcept -> PacketWrapper & {
    if (this != &other) {
      av_packet_unref(pack);   // free current packet if any
      pack = other.pack;       // steal the pointer
      other.pack = nullptr;    // other no longer owns it
    }
    return *this;
  }
  
  PacketWrapper::~PacketWrapper() { av_packet_unref(pack); }
}

