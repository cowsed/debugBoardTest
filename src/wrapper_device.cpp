#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "vdb/protocol.hpp"
#include "wrapper_device.hpp"

namespace VDB {
void delay_ms(uint32_t ms) { vexDelay(ms); }
uint32_t time_ms() { return vexSystemTimeGet(); }

Device::Device(int32_t port, int32_t baud_rate)
    : COBSSerialDevice(port, baud_rate) {}

bool Device::send_packet(const VDP::Packet &packet) {
  return COBSSerialDevice::send_cobs_packet(packet);
}
void Device::register_receive_callback(
    std::function<void(const VDP::Packet &packet)> new_callback) {
  callback = std::move(new_callback);
}
void Device::cobs_packet_callback(const Packet &pac) { callback(pac); }

} // namespace VDB
