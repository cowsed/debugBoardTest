#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "vdb/protocol.hpp"
#include "wrapper_device.hpp"

namespace VDB {
void delay_ms(uint32_t ms) { vexDelay(ms); }
uint32_t time_ms() { return vexSystemTimeGet(); }

Device::Device(COBSSerialDevice &underlying_device)
    : underlying(underlying_device) {}

bool Device::send_packet(const VDP::Packet &packet) {
  return underlying.send_packet(packet);
}
void Device::register_receive_callback(
    std::function<void(const VDP::Packet &packet)> callback) {
  underlying.register_recieve_callback(callback);
}

} // namespace VDB
