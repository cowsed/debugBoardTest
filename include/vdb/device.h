#pragma once
#include "cobs_device.h"
#include "vdb/protocol.h"
#include "vex.h"

namespace VDB {
class Device : public VDP::AbstractDevice {
public:
  Device(COBSSerialDevice &underlying);
  bool send_packet(const VDP::Packet &packet) override;
  void register_receive_callback(
      std::function<void(const VDP::Packet &packet)> callback);

private:
  COBSSerialDevice &underlying;
};

} // namespace VDB
