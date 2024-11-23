#pragma once
#include <cstdint>
#include <deque>
#include <functional>
#include <vector>
#include <vex.h>

class COBSSerialDevice {
public:
  using WirePacket = std::vector<uint8_t>; // 0x00 delimeted, cobs encoded
  using Packet = std::vector<uint8_t>;

  static constexpr std::size_t MAX_OUT_QUEUE_SIZE = 50;
  static constexpr std::size_t MAX_IN_QUEUE_SIZE = 50;
  static constexpr std::size_t baud_rate = 115200 * 4;

  COBSSerialDevice(uint32_t port);

  bool send_packet(const Packet &pac);
  void register_recieve_callback(std::function<void(const Packet &)> cb);

  // private:
  uint32_t port;
  std::function<void(const Packet &)> packet_callback;

  /// @brief Packets that have been encoded and are waiting for their turn
  /// to be sent out on the wire
  std::deque<WirePacket> outbound_packets{};
  vex::mutex outbound_mutex;

  /// @brief Packets that have been read from the wire and split up but that are
  /// still COBS encoded
  std::deque<WirePacket> inbound_packets;
  vex::mutex inbound_mutex;
  /// @brief Working buffer that the reading thread uses to assemble packets
  /// until it finds a full COBS packet
  WirePacket inbound_buffer;

  static void cobs_encode(const Packet &in, WirePacket &out);
  static void cobs_decode(const WirePacket &in, Packet &out);
  void handle_inbound_byte(uint8_t b);
  bool write_packet_if_avail();

  // Task that deals with the low level writing and reading bytes from the wire
  vex::task serial_task;
  static int serial_thread(void *self);

  // Once the serial_task has read in an entire packet, it must be decoded
  // this thread decodes it back into its original binary form and calls the
  // user callback
  vex::task decode_task;
  static int decode_thread(void *self);
};
