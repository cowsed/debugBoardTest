#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "cobs_device.h"
#include "vdb/device.h"
#include "vdb/protocol.h"
namespace VDB {
Device::Device(COBSSerialDevice &underlying) : underlying(underlying) {}

bool Device::send_packet(const VDP::Packet &packet) {
  return underlying.send_packet(packet);
}
void Device::register_receive_callback(
    std::function<void(const VDP::Packet &packet)> callback) {
  underlying.register_recieve_callback(callback);
}

// Dummy child of vexlink to get access to their crc32 checksum calculation
// function
class dummy_vexlink : vex::vexlink {
public:
  static unsigned int crc32_one(unsigned int a, unsigned char c) {
    return vexlink::crc32(&c, 1, a);
  }
  static unsigned int crc32_buf(unsigned int a, const unsigned char *c,
                                uint32_t len) {
    // const cast :skull:
    return vexlink::crc32(const_cast<uint8_t *>(c), len, a);
  }
};

} // namespace VDB

namespace VDP {

uint32_t crc32_one(uint32_t accum, uint8_t b) {
  return VDB::dummy_vexlink::crc32_one(accum, b);
}
uint32_t crc32_buf(uint32_t accum, const uint8_t *b, uint32_t length) {
  return VDB::dummy_vexlink::crc32_buf(accum, b, length);
}
} // namespace VDP
