#pragma once

#include "vdb/protocol.h"
#include "vdb/registry.h"
#include "vex.h"
#include <deque>
#include <unordered_map>

namespace VDB {

class Device {
public:
  using WirePacket = std::vector<uint8_t>; // 0x00 delimeted, cobs encoded

  static constexpr size_t MAX_OUT_QUEUE_SIZE = 50;
  static constexpr size_t MAX_IN_QUEUE_SIZE = 50;
  static constexpr size_t baud_rate = 115200;

  Device(int32_t port, VDP::Registry &reg);

  void send_packet(const VDP::Packet &pac);
  VDP::Registry &get_registry() { return reg; }

private:
  VDP::Registry &reg;

  /// Thread function for the direct reading and writing to the serial port
  static int hardware_thread(void *self);
  static int decoder_thread(void *self);

  static void CobsEncode(const VDP::Packet &in, WirePacket &out);
  static void CobsDecode(const WirePacket &in, VDP::Packet &out);
  void handle_inbound_byte(uint8_t b);
  bool write_packet();

  int32_t port;
  /// @brief Packets that have been encoded and are waiting for their turn to be
  /// sent out on the wire
  std::deque<WirePacket> outbound_packets;
  vex::mutex outbound_mutex;

  /// @brief Packets that have been read from the wire and split up but that are
  /// still COBS encoded
  std::deque<WirePacket> inbound_packets;
  vex::mutex inbound_mutex;
  /// @brief Working buffer that the reading thread uses to assemble packets
  /// until it finds a full COBS packet
  WirePacket inbound_buffer;

  vex::task hw_task;
  vex::task decode_task;
};
} // namespace VDB
