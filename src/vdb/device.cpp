#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "cobs_device.hpp"
#include "vdb/device.hpp"
#include "vdb/protocol.hpp"
namespace VDB {
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
