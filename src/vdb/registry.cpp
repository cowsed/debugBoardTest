#include "vdb/registry.h"

namespace VDP {
void Registry::install_broadcast_callback(CallbackFn on_broadcast) {
  this->on_broadcast = std::move(on_broadcast);
}
void Registry::install_data_callback(CallbackFn on_data) {
  this->on_data = std::move(on_data);
}

VDP::PacketValidity validate_packet(const VDP::Packet &packet) {
  if (packet.size() < 6) {
    // packet header byte + channel byte + checksum = 6 bytes
    return VDP::PacketValidity::TooSmall;
  }
  auto checksum = crc32_buf(0, packet.data(), packet.size() - 4);
  auto size = packet.size();
  const uint32_t written_checksum = (packet[size - 1] << 24) |
                                    (packet[size - 2] << 16) |
                                    (packet[size - 3] << 8) | packet[size - 4];
  // printf("%04lx vs %04lx\n", checksum, written_checksum);
  if (checksum != written_checksum) {
    return VDP::PacketValidity::BadChecksum;
  }
  return VDP::PacketValidity::Ok;
}

void Registry::take_packet(const Packet &pac) {
  const VDP::PacketValidity status = validate_packet(pac);

  if (status == VDP::PacketValidity::BadChecksum) {
    printf("Bad packet checksum. Skipping\n");
    return;
  } else if (status == VDP::PacketValidity::TooSmall) {
    printf("Packet too small to be valid. Skipping\n");
    return;
  } else if (status != VDP::PacketValidity::Ok) {
    printf("Unknown validity of packet (THIS SHOULD NOT HAPPEN). Skipping\n");
    return;
  }

  const VDP::PacketHeader header = VDP::decode_header_byte(pac[0]);
  if (header.func == VDP::PacketFunction::Send) {
    if (header.type == VDP::PacketType::Broadcast) {
      VDP::Channel chan = VDP::decode_broadcast(pac);
      on_broadcast(chan);
    } else {
      ChannelID id = pac[1];
      printf("UNIMPLEMENTED DATA for chan %d\n", id);
    }
  } else {
    printf("UNIMPLEMENTED: ACKs\n");
  }
}

} // namespace VDP