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
#include <vector>
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

// #define MIKE_BUFLEN 32
// #define SERIAL_PORT PORT1

// int main() {
//   static uint8_t buf[MIKE_BUFLEN] = {0};
//   Brain.Screen.printAt(10, 50, "Hello V5");
//   vexGenericSerialEnable(PORT1, 0x0);
//   vexGenericSerialBaudrate(PORT1, 115200);

//   vexGenericSerialEnable(PORT6, 0x0);
//   vexGenericSerialBaudrate(PORT6, 115200);

//   while (1) {
//     // dev1.write_packet({'b'});
//     uint8_t sendbuf[] = {'a', 'b', 'c'};
//     vexGenericSerialTransmit(PORT1, sendbuf, 3);
//     int i = vexGenericSerialWriteChar(PORT1, 'a');
//     printf("printed : %d\n", i);
//     vexDelay(10);
//     i = vexGenericSerialReceive(PORT6, buf, MIKE_BUFLEN);
//     printf("Recieved: %lu at %d\n", i, vexSystemTimeGet());
//     if (i > 0) {
//       printf("Message: %.*s\n", i, buf);
//     }
//     // Allow other tasks to run
//     this_thread::sleep_for(500);
//   }
// }

VDB::Device dev1(PORT1);
VDB::Device dev2(PORT6);

int main() {
  mot1.spin(vex::fwd, 2.0, vex::voltageUnits::volt);
  VDP::Schema::PartPtr schema{new VDP::Schema::Motor("Motor 1", mot1)};

  VDP::Schema::PacketWriter writer;
  schema->write_schema(writer);

  VDP::Packet pac{writer.get_packet()};
  VDP::Schema::PartPtr rev = VDP::decode_schema(pac);

  vex::timer tmr;
  uint32_t num_written = 0;
  while (tmr.time(vex::seconds) < 5) {
    schema->fetch();
    writer.write_message(schema);

    VDP::Packet msg = writer.get_packet();
    // VDP::dump_packet(msg);

    VDP::Schema::PacketReader reader{msg};
    schema->read_from_message(reader);

    std::string data_str = schema->pretty_print_data();

    dev1.send_packet(writer.get_packet());
    // dev2.write_packet(writer.get_packet());
    num_written++;
    // printf("Data:\n%s", data_str.c_str());
    vexDelay(1);
  }
  printf("Wrote %lu packets in 5 seconds\n", num_written);
  fflush(stdout);
  mot1.stop();
}