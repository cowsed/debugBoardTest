/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Module:       main.cpp                                                  */
/*    Author:       unknown                                                   */
/*    Created:      7/7/2024, 3:53:01 PM                                      */
/*    Description:  V5 project                                                */
/*                                                                            */
/*----------------------------------------------------------------------------*/
#include "vdb/builtins.h"
#include "vdb/device.h"
#include "vdb/protocol.h"
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

vex::motor mot1{vex::PORT11};

static constexpr VDP::ChannelID motor_chan_id = 1;

VDP::Registry reg1;
// VDP::Registry reg2;

VDB::Device dev1(PORT1, reg1);
// VDB::Device dev2(PORT1, reg2);

int main() {
  mot1.spin(vex::fwd, 2.0, vex::voltageUnits::volt);
  const VDP::Channel motor_channel = {
      motor_chan_id,
      (VDP::PartPtr) new VDP::Motor("Motor 1", mot1),
  };

  VDP::PacketWriter writer;
  writer.write_channel_broadcast(motor_channel);
  dev1.send_packet(writer.get_packet());
  const VDP::Packet pac{writer.get_packet()};

  vex::timer tmr;
  uint32_t num_written = 0;

  while (tmr.time(vex::seconds) < 500) {
    motor_channel.data->fetch();
    writer.write_message(motor_channel);

    const VDP::Packet msg = writer.get_packet();
    // VDP::dump_packet(msg);

    // VDP::PacketReader reader{msg};

    // // schema->read_from_message(reader);

    std::string str = motor_channel.data->pretty_print_data();
    Brain.Screen.setCursor(1, 1);
    Brain.Screen.clearScreen();
    int line_start = 0;
    for (size_t i = 0; i < str.size(); i++) {
      if (str[i] == '\n') {
        std::string line = str.substr(line_start, i - line_start);
        Brain.Screen.print("%s", line.c_str());
        Brain.Screen.newLine();
        line_start = i + 1;
      } else if (str[i] == '\r') {
        str[i] = ' ';
      } else if (str[i] == '\t') {
        str[i] = ' ';
      }
    }
    Brain.Screen.render(false, true);

    dev1.send_packet(writer.get_packet());
    // dev2.write_packet(writer.get_packet());
    num_written++;
    // printf("Data:\n%s", data_str.c_str());
    vexDelay(1000);
  }
  printf("Wrote %lu packets in 5 seconds\n", num_written);
  fflush(stdout);
  mot1.stop();
}