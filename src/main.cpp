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
#include "vdb/registry.h"
#include "vdb/tests.h"

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
#define FLUSHWAIT                                                              \
  fflush(stdout);                                                              \
  vexDelay(1000);

void print_multiline(std::string str, int y, int x);

int main() {
  bool ok = VDP::test_all();
  if (!ok) {
    printf("STOP THE PRESSES THE TESTS FAILED\n");
  } else {
    printf("Tests pass\n");
  }

  auto u1 = COBSSerialDevice(PORT1);
  auto u2 = COBSSerialDevice(PORT6);
  VDB::Device dev1{u1};
  VDB::Device dev2{u2};
  VDP::Registry reg1{&dev1};
  VDP::Registry reg2{&dev2};

  vexDelay(1);

  printf("opening channel\n");
  VDP::Channel chan1 = reg1.open_channel((VDP::PartPtr) new VDP::Float(
      "my float", []() { return (float)vexSystemTimeGet(); }));

  vexDelay(10000);
  return 0;

  // VDP::PacketWriter writer;
  // writer.write_channel_broadcast(motor_channel);
  // dev1.send_packet(writer.get_packet());
  // const VDP::Packet pac{writer.get_packet()};

  // vex::timer tmr;
  // uint32_t num_written = 0;

  // while (tmr.time(vex::seconds) < 500) {
  //   motor_channel.data->fetch();
  //   writer.write_message(motor_channel);

  //   const VDP::Packet msg = writer.get_packet();
  //   VDP::dump_packet(msg);

  //   // VDP::PacketReader reader{msg};
  //   // motor_channel.data
  //   // schema->read_from_message(reader);

  //   std::string str = motor_channel.data->pretty_print_data();
  //   Brain.Screen.clearScreen();
  //   print_multiline(str, 1, 1);

  //   std::string str2 = motor_channel.data->pretty_print_data();
  //   print_multiline(str2, 1, 24);

  //   Brain.Screen.render(false, true);

  //   dev1.send_packet(writer.get_packet());
  //   // dev2.write_packet(writer.get_packet());
  //   num_written++;
  //   // printf("Data:\n%s", data_str.c_str());
  //   vexDelay(1000);
  // }
  // printf("Wrote %lu packets in 5 seconds\n", num_written);
  // fflush(stdout);
  // mot1.stop();
}

void print_multiline(std::string str, int y, int x) {
  int line_count = 0;
  int line_start = 0;
  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == '\n') {
      std::string line = str.substr(line_start, i - line_start);
      Brain.Screen.setCursor(y + line_count, x);
      Brain.Screen.print("%s", line.c_str());
      line_count++;
      line_start = i + 1;
    } else if (str[i] == '\r') {
      str[i] = ' ';
    } else if (str[i] == '\t') {
      str[i] = ' ';
    }
  }
}
