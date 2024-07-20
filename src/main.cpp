/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Module:       main.cpp                                                  */
/*    Author:       unknown                                                   */
/*    Created:      7/7/2024, 3:53:01 PM                                      */
/*    Description:  V5 project                                                */
/*                                                                            */
/*----------------------------------------------------------------------------*/
#include "vdp.h"
#include "vdp_builtins.h"
#include "vdp_device.h"
#include "vex.h"
using namespace vex;

// A global instance of vex::brain used for printing to the V5 brain screen
vex::brain Brain;

/*
brain = peripheral
esp = controller

advertise message, data message
controller->peripheral (esp to brain)
preipheral->controller (brain to esp)

types:
- string (null terminated)
- double
  8 bytes, a classic
- uint64
- byte
- list
  length terminated
- keyed-list
  length terminated: keys in schema

target: id: subsystem
- channel: data
  - datatype


Header:
is_peripheral: 1
type: 1
version: 6

Advertise
1tvvvvvv : Header
xxxxxxxx : Number of Fields (0-255) (N)
N Times:
  8xM-0    : Null terminated name of this channel

*/
#define FLUSH                                                                  \
  fflush(stdout);                                                              \
  vexDelay(1000);

vex::motor mot1{vex::PORT10};

static constexpr VDP::ChannelID motor_chan_id = 1;

VDB::Device dev1(PORT1);
VDB::Device dev2(PORT6);
int main() {
  dev2.install_broadcast_callback([](VDP::Channel chan) {
    printf("dev2 broadcast received for channel %d!\n", chan.id);
  });
  dev2.install_data_callback([](VDP::Channel chan) {
    printf("dev2 data received for channel %d!\n", chan.id);
  });

  mot1.spin(vex::fwd, 2.0, vex::voltageUnits::volt);
  const VDP::Channel motor_channel = {
      motor_chan_id,
      (VDP::PartPtr) new VDP::Motor("Motor 1", mot1),
  };

  VDP::PacketWriter writer;
  writer.write_channel_broadcast(motor_channel);
  dev1.send_packet(writer.get_packet());

  const VDP::Packet pac{writer.get_packet()};

  // VDP::PartPtr schema = rev.data;
  vex::timer tmr;
  uint32_t num_written = 0;
  // return 0;
  while (tmr.time(vex::seconds) < 5) {
    motor_channel.data->fetch();
    writer.write_message(motor_channel);

    const VDP::Packet msg = writer.get_packet();
    // VDP::dump_packet(msg);

    // VDP::PacketReader reader{msg};

    // // schema->read_from_message(reader);

    const std::string data_str = motor_channel.data->pretty_print_data();

    dev1.send_packet(writer.get_packet());
    // // dev2.write_packet(writer.get_packet());
    num_written++;
    // printf("Data:\n%s", data_str.c_str());
    vexDelay(1);
  }
  printf("Wrote %lu packets in 5 seconds\n", num_written);
  fflush(stdout);
  mot1.stop();
}