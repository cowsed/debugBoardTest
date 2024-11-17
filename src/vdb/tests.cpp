#include "vdb/builtins.h"
#include "vdb/device.h"
#include "vdb/protocol.h"
#include "vdb/registry.h"
namespace VDP {

class SilentDevice : public AbstractDevice {
public:
  void send_packet(const VDP::Packet &) override {};
  void register_receive_callback(
      std::function<void(const VDP::Packet &)>) override{};
};
namespace RegistryTest {

bool test_broadcast() {
  VDP::SilentDevice dev;
  VDP::Registry reg_out{&dev};
  VDP::Registry reg_in{&dev};

  vex::motor mot1{vex::PORT21};
  mot1.spin(vex::fwd, 2.0, vex::voltageUnits::volt);
  const VDP::Channel motor_channel =
      reg_out.open_channel((VDP::PartPtr) new VDP::Motor("Motor 1", mot1));

  VDP::PacketWriter writer;
  bool was_broadcast_correctly = false;
  reg_in.install_broadcast_callback([&](VDP::Channel chan) {
    if (chan.id != motor_channel.id) {
      was_broadcast_correctly = false;
      return;
    }
    auto schema_out = motor_channel.data->pretty_print();
    auto schema_in = chan.data->pretty_print();
    if (schema_in != schema_out) {
      was_broadcast_correctly = false;
      return;
    }
    was_broadcast_correctly = true;
  });
  writer.write_channel_broadcast(motor_channel);
  VDP::Packet packet = writer.get_packet();
  reg_in.take_packet(packet);

  return was_broadcast_correctly;
}
} // namespace RegistryTest
bool test_all() {
  printf("Testin\n");
  vexDelay(100);
  fflush(stdout);
  using Test = std::pair<const char *, bool (*)()>;
  std::array<Test, 1> tests = {
      Test{"Test Broadcast", RegistryTest::test_broadcast},
  };

  bool all_passed = true;
  for (Test test : tests) {
    bool res = test.second();
    if (res == false) {
      printf("Test '%s' failed\n", test.first);
    }
  }
  return all_passed;
}

} // namespace VDP

bool test_enc_dec(const VDP::Channel &chan) {
  VDP::PacketWriter writ;
  writ.write_channel_broadcast(chan);
  VDP::Packet packet = writ.get_packet();
  bool ok = true;

  ok &= (packet[0] == 0);
  ok &= (packet[1] == chan.id);
  VDP::dump_packet(packet);

  return ok;
}
