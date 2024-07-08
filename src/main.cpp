/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Module:       main.cpp                                                  */
/*    Author:       unknown                                                   */
/*    Created:      7/7/2024, 3:53:01 PM                                      */
/*    Description:  V5 project                                                */
/*                                                                            */
/*----------------------------------------------------------------------------*/
#include "vdp.h"
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
  vexDelay(500);

int main() {
  printf("helloo\n");
  FLUSH;

  VDP::Schema::Part *p = new VDP::Schema::Double("example value");
  VDP::Schema::Part *p2 = new VDP::Schema::String();
  VDP::Packet pac{};

  p->add_to_schema(pac);

  int i = 0;
  for (const uint8_t d : pac) {
    printf("%02x ", (int)d);
    if (i % 16 == 0) {
      printf("\n");
    }
    i++;
  }
  printf("\n");
  // printf("Done");
  // FLUSH
}