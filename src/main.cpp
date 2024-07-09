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

int main() {
  vexDisplayPrintf(10, 10, true, "Hello");
  VDP::Schema::PartPtr schema{new VDP::Schema::Motor("Motor 1", mot1)};

  vexDisplayPrintf(10, 10, true, "Made Schema");

  std::string schema_str = schema->pretty_print();

  vexDisplayPrintf(10, 10, true, "PPRinted schema");

  vexDisplayPrintf(10, 50, true, schema_str.c_str());

  printf("Schema In:\n%s", schema_str.c_str());

  VDP::Packet pac{};

  schema->write_to_schema(pac);

  VDP::dump_packet(pac);

  VDP::Schema::PartPtr rev = VDP::decode_schema(std::move(pac));
  std::string rev_str = schema->pretty_print();

  printf("Schema Out:\n%s", rev_str.c_str());

  // printf("Done");
  // FLUSH
}