#include "vdb/registry.h"

namespace VDP {
Registry::Registry(AbstractDevice *device, Side reg_type)
    : reg_type(reg_type), device(device) {
  device->register_receive_callback([&](const Packet &p) { take_packet(p); });

  my_channels.resize(256);
  remote_channels.resize(256);
  for (auto &c : my_channels) {
    c = Channel{0, nullptr};
  }
  for (auto &c : remote_channels) {
    c = Channel{0, nullptr};
  }
}

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
  auto checksum = crc32_buf(0xFFFFFFFF, packet.data(), packet.size() - 4);
  auto size = packet.size();
  const uint32_t written_checksum = (packet[size - 1] << 24) |
                                    (packet[size - 2] << 16) |
                                    (packet[size - 3] << 8) | packet[size - 4];
  if (checksum != written_checksum) {
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
  const VDP::PacketValidity status = validate_packet(pac);

  if (status == VDP::PacketValidity::BadChecksum) {
    // printf("Bad packet checksum. Skipping\n");
    num_bad++;
    return;
  } else if (status == VDP::PacketValidity::TooSmall) {
    num_small++;
    // printf("Packet too small to be valid. Skipping\n");
    return;
  } else if (status != VDP::PacketValidity::Ok) {
    printf("Unknown validity of packet (THIS SHOULD NOT HAPPEN). Skipping\n");
    return;
  }

  const VDP::PacketHeader header = VDP::decode_header_byte(pac[0]);
  if (header.func == VDP::PacketFunction::Send) {
    if (header.type == VDP::PacketType::Broadcast) {
      const VDP::Channel chan = VDP::decode_broadcast(pac);
      remote_channels[chan.id] = chan;
      printf("VDB-%s: Got broadcast of channel %d\n",
             (reg_type == Side::Controller ? "Controller" : "Listener"),
             int(chan.id));
      on_broadcast(chan);

      PacketWriter writer;
      writer.write_channel_acknowledge(chan);
      device->send_packet(writer.get_packet());

    } else if (header.type == VDP::PacketType::Data) {
      const ChannelID id = pac[1];
      const PartPtr part = get_remote_schema(id);
      if (part == nullptr) {
        printf("VDB-%s: No channel information for id: %d\n",
               (reg_type == Side::Controller ? "Controller" : "Listener"), id);
        return;
      }
      PacketReader reader{pac, 2};
      part->read_data_from_message(reader);
      on_data(Channel{id, part});
    }
  } else {
    PacketReader reader(pac);
    // header byte, had to be read to know were a braodcast
    (void)reader.get_byte();
    const ChannelID id = reader.get_number<ChannelID>();
    if (id != highest_acked_channel + 1) {
      printf("ACK out of order?????");
      return;
    }
    needs_ack = false;
    highest_acked_channel++;
  }
}

Channel Registry::open_channel(PartPtr for_data) {
  ChannelID id = new_channel_id();
  Channel chan = Channel{id, for_data};

  if (needs_ack && waiting_on_ack_timer.time(vex::timeUnits::msec) < ack_ms) {
    printf("Still waiting on ack");
    vexDelay(ack_ms - waiting_on_ack_timer.time(vex::timeUnits::msec));
  }

  if (reg_type == Side::Controller) {
    PacketWriter writer;
    writer.write_channel_broadcast(chan);
    VDP::Packet pac = writer.get_packet();

    device->send_packet(pac);
    needs_ack = true;
    waiting_on_ack_timer.reset();
  }
  return chan;
}

bool Registry::send_data(const Channel &data) {
  if (data.id > next_channel_id) {
    printf("VDB-%s: Channel with ID %d doesn't exist yet\n",
           (reg_type == Side::Controller ? "Controller" : "Listener"),
           (int)data.id);
    return false;
  }
  if (data.id > highest_acked_channel) {
    printf("VDB-%s: Channel %d has not yet been negotiated. Dropping packet\n",
           (reg_type == Side::Controller ? "Controller" : "Listener"),
           (int)data.id);
    return false;
  }

  PacketWriter writ;

  writ.write_message(data);
  VDP::Packet pac = writ.get_packet();
  return device->send_packet(pac);
}

} // namespace VDP