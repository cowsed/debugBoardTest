#pragma once
#include "cobs_device.hpp"
#include "vdb/protocol.hpp"
#include "vex.h"

namespace VDB {
class Device : public VDP::AbstractDevice {
public:
  explicit Device(COBSSerialDevice &underlying);
  bool send_packet(const VDP::Packet &packet) override;
  void register_receive_callback(
      std::function<void(const VDP::Packet &packet)> callback) override;

private:
  COBSSerialDevice &underlying;
};

} // namespace VDB
