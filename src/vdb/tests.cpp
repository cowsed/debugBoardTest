#include "vdb/tests.hpp"
#include "vdb/builtins.hpp"
#include "vdb/device.hpp"
#include "vdb/protocol.hpp"
#include "vdb/registry.hpp"
namespace VDP {

class SilentDevice : public AbstractDevice {
public:
  bool send_packet(const VDP::Packet &) override { return true; }
  void
  register_receive_callback(std::function<void(const VDP::Packet &)>) override {
  }
};
namespace RegistryTest {

static bool test_broadcast() {
  VDP::SilentDevice dev;
  VDP::Registry reg_out{&dev, VDP::Registry::Side::Controller};
  VDP::Registry reg_in{&dev, VDP::Registry::Side::Listener};

  vex::motor mot1{vex::PORT21};
  // const VDP::Channel motor_channel =
  // reg_out.open_channel((VDP::PartPtr) new VDP::Motor("Motor 1", mot1));
  //
  // VDP::PacketWriter writer;
  bool was_broadcast_correctly = false;
  // reg_in.install_broadcast_callback([&](VDP::Channel chan) {
  // if (chan.getID() != motor_channel.getID()) {
  // was_broadcast_correctly = false;
  // return;
  // }
  // auto schema_out = motor_channel.data->pretty_print();
  // auto schema_in = chan.data->pretty_print();
  // if (schema_in != schema_out) {
  // was_broadcast_correctly = false;
  // return;
  // }
  // was_broadcast_correctly = true;
  // });
  // writer.write_channel_broadcast(motor_channel);
  // VDP::Packet packet = writer.get_packet();
  // reg_in.take_packet(packet);

  return was_broadcast_correctly;
}
} // namespace RegistryTest
bool test_all() {
  using Test = std::pair<const char *, bool (*)()>;
  std::array<Test, 1> tests = {
      Test{"Test Broadcast", RegistryTest::test_broadcast},
  };

  bool all_passed = true;
  for (Test test : tests) {
    bool res = test.second();
    if (res == false) {
      printf("Test '%s' failed\n", test.first);
      all_passed = false;
    }
  }
  return all_passed;
}

} // namespace VDP
