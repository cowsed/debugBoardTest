#pragma once

#include "vdp.h"
#include "vex.h"
#include <deque>
#include <unordered_map>

namespace VDB {
uint32_t crc32_one(uint32_t accum, uint8_t b);
uint32_t crc32_buf(uint32_t accum, const uint8_t *b, uint32_t length);

class Device {
public:
  using CallbackFn = std::function<void(VDP::Channel)>;
  using WirePacket = std::vector<uint8_t>; // 0x00 delimeted, cobs encoded

  static constexpr size_t MAX_OUT_QUEUE_SIZE = 50;
  static constexpr size_t MAX_IN_QUEUE_SIZE = 50;
  static constexpr size_t baud_rate = 115200;

  Device(int32_t port);

  void send_packet(const VDP::Packet &pac);
  void install_broadcast_callback(CallbackFn on_broadcast);
  void install_data_callback(CallbackFn on_data);

private:
  /// Thread function for the direct reading and writing to the serial port
  static int hardware_thread(void *self);
  static int decoder_thread(void *self);

  static void CobsEncode(const VDP::Packet &in, WirePacket &out);
  static void CobsDecode(const WirePacket &in, VDP::Packet &out);
  void handle_inbound_byte(uint8_t b);
  bool write_packet();

  CallbackFn on_broadcast;
  CallbackFn on_data;

  std::vector<VDP::Channel> remote_channels;

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

// // define your global instances of motors and other devices here
// #define MIKE_BUFLEN 32
// #define SERIAL_PORT PORT1
// int main() {
//   static uint8_t buf[MIKE_BUFLEN] = {0};
//   Brain.Screen.printAt(10, 50, "Hello V5");
//   vexGenericSerialEnable(PORT1, 0x0);
//   vexGenericSerialBaudrate(PORT1, 115200);
//   while (1) {
//     int i = vexGenericSerialWriteChar(SERIAL_PORT, 'a');
//     printf("printed : %d\n", i);

//     i = vexGenericSerialReceive(SERIAL_PORT, buf, MIKE_BUFLEN);
//     printf("Recieved: %d at %d\n", i, vexSystemTimeGet());
//     if (i > 0) {
//       printf("Message: %.*s\n", i, buf);
//     }
//     // Allow other tasks to run
//     this_thread::sleep_for(500);
//   }
// }
