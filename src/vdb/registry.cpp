#include "vdb/registry.hpp"

namespace VDP {
Registry::Registry(AbstractDevice *device, Side reg_type)
    : reg_type(reg_type), device(device) {
  device->register_receive_callback([&](const Packet &p) { take_packet(p); });
}

const char *Registry::identifier() {
  return (reg_type == Side::Controller ? "VDB:Controller" : "VDB:Listener");
}

void Registry::install_broadcast_callback(CallbackFn on_broadcastf) {
  VDPTracef("%s: Installed broadcast callback for ", identifier());
  this->on_broadcast = std::move(on_broadcastf);
}
void Registry::install_data_callback(CallbackFn on_dataf) {
  VDPTracef("%s: Installed data callback for ", identifier());
  this->on_data = std::move(on_dataf);
}

static VDP::PacketValidity validate_packet(const VDP::Packet &packet) {
  VDPTracef("Validating packet of size %d", (int)packet.size());

  // packet header byte + channel byte + checksum = 6 bytes
  static constexpr size_t min_packet_size = 6;

  if (packet.size() < min_packet_size) {
    return VDP::PacketValidity::TooSmall;
  }

  uint32_t checksum = CRC32::calculate(packet.data(), packet.size() - 4);

  auto size = packet.size();
  const uint32_t written_checksum =
      (uint32_t(packet[size - 1]) << 24) | (uint32_t(packet[size - 2]) << 16) |
      (uint32_t(packet[size - 3]) << 8) | uint32_t(packet[size - 4]);

  if (checksum != written_checksum) {
    VDPWarnf("Checksums do not match: expected: %08lx, got: %08lx", checksum,
             written_checksum);
    return VDP::PacketValidity::BadChecksum;
  }
  return VDP::PacketValidity::Ok;
}

PartPtr Registry::get_remote_schema(ChannelID id) {
  if (id >= remote_channels.size()) {
    return nullptr;
  }
  return remote_channels[id].data;
}

void Registry::take_packet(const Packet &pac) {
  VDPTracef("Received packet of size %d", (int)pac.size());

  const VDP::PacketValidity status = validate_packet(pac);

  if (status == VDP::PacketValidity::BadChecksum) {
    VDPWarnf("%s: Bad packet checksum. Skipping", identifier());
    num_bad++;
    return;
  } else if (status == VDP::PacketValidity::TooSmall) {
    num_small++;
    VDPWarnf("%s: Packet too small to be valid (%d bytes). Skipping",
             identifier(), (int)pac.size());
    dump_packet(pac);
    return;
  } else if (status != VDP::PacketValidity::Ok) {
    VDPWarnf("%s: Unknown validity of packet (BAD). Skipping", identifier());
    return;
  }

  const VDP::PacketHeader header = VDP::decode_header_byte(pac[0]);

  if (header.func == VDP::PacketFunction::Send) {
    VDPTracef("%s: PacketFunction Send", identifier());

    if (header.type == VDP::PacketType::Broadcast) {
      VDPTracef("%s: PacketType Broadcast", identifier());
      auto decoded = VDP::decode_broadcast(pac);

      VDP::Channel chan{decoded.second, decoded.first};

      if (remote_channels.size() < chan.id) {
        VDPWarnf("%s: Out of order broadcast. dropping", identifier());
        return;
      }
      remote_channels.push_back(chan);
      VDPTracef("%s: Got broadcast of channel %d", identifier(), int(chan.id));
      on_broadcast(chan);

      Packet scratch;
      PacketWriter writer{scratch};
      writer.write_channel_acknowledge(chan);
      device->send_packet(writer.get_packet());

    } else if (header.type == VDP::PacketType::Data) {
      VDPTracef("%s: PacketType Data", identifier());
      const ChannelID id = pac[1];
      const PartPtr part = get_remote_schema(id);
      if (part == nullptr) {
        VDPDebugf("VDB-%s: No channel information for id: %d", identifier(),
                  id);
        return;
      }
      PacketReader reader{pac, 2};
      part->read_data_from_message(reader);
      on_data(Channel{part, id});
    }
  } else if (header.func == VDP::PacketFunction::Acknowledge) {
    PacketReader reader(pac);
    // header byte, had to be read to know were a braodcast
    (void)reader.get_byte();
    const ChannelID id = reader.get_number<ChannelID>();
    if (id >= my_channels.size()) {
      printf("VDB-%s: Recieved ack for unknown channel %d",
             (reg_type == Side::Controller ? "Controller" : "Listener"), id);
    }
    my_channels[id].acked = true;
  }
}

bool Registry::negotiate() {
  if (reg_type != Side::Controller) {
    return false;
  }
  printf("Negotiating\n");
  bool acked_all = true;
  int failed_acks = 0;

  size_t broadcast_tries_per = 3;

  for (size_t i = 0; i < my_channels.size(); i++) {
    for (size_t j = 0; j < broadcast_tries_per; j++) {
      VDPDebugf("%s: Negotiating chan id %d", identifier(), i);

      const Channel &chan = my_channels[i];
      Packet scratch;
      PacketWriter writer{scratch};
      writer.write_channel_broadcast(chan);
      VDP::Packet pac = writer.get_packet();

      device->send_packet(pac);
      needs_ack = true;

      auto time = VDB::time_ms();
      while (!chan.acked) {
        auto now = VDB::time_ms();
        if (now - time > ack_ms) {
          // timed out
          break;
        }
        VDB::delay_ms(5);
      }
      if (chan.acked == true) {
        VDPTracef("%s: Acked channel %d after %d ms on attempt %d",
                  identifier(), chan.id, (int)(VDB::time_ms() - time),
                  (int)j + 1);
        break;
      } else {
        VDPWarnf("%s: ack for chan id:%02x expired after %d msec", identifier(),
                 chan.id, (int)ack_ms);
        failed_acks++;
        if (j == broadcast_tries_per - 1) {
          acked_all = false;
        }
      }
    }
  }
  if (failed_acks > 0) {
    VDPWarnf("%s: Failed to ack %d times", identifier(), failed_acks);
  }
  return acked_all;

  // if (needs_ack && waiting_on_ack_timer.time(vex::timeUnits::msec) < ack_ms)
  // { printf("Still waiting on ack"); vexDelay(ack_ms -
  // waiting_on_ack_timer.time(vex::timeUnits::msec));
  // }

  // if (reg_type == Side::Controller) {
  // PacketWriter writer;
  // writer.write_channel_broadcast(chan);
  // VDP::Packet pac = writer.get_packet();
  //
  // device->send_packet(pac);
  // needs_ack = true;
  // waiting_on_ack_timer.reset();
  // }
}
ChannelID Registry::open_channel(PartPtr for_data) {
  ChannelID id = new_channel_id();
  Channel chan = Channel{for_data, id};
  my_channels.push_back(chan);
  return chan.id;
}

bool Registry::send_data(ChannelID id, PartPtr data) {
  if (id > my_channels.size()) {
    printf("VDB-%s: Channel with ID %d doesn't exist yet\n",
           (reg_type == Side::Controller ? "Controller" : "Listener"), (int)id);
    return false;
  }
  Channel &chan = my_channels[id];
  chan.data = data;

  if (!chan.acked) {
    printf("VDB-%s: Channel %d has not yet been negotiated. Dropping packet\n",
           (reg_type == Side::Controller ? "Controller" : "Listener"), (int)id);
    return false;
  }
  VDP::Packet scratch;
  PacketWriter writ{scratch};

  writ.write_data_message(chan);
  VDP::Packet pac = writ.get_packet();

  return device->send_packet(pac);
}

} // namespace VDP
