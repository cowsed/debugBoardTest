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

VDP::Schema::PartPtr make_schema() {
  using namespace VDP::Schema;
  VDP::Schema::PartPtr schema{
      new Record("Motor", {
                              new String("BrakeMode"),
                              new Double("Position(deg)"),
                              new Double("Velocity(dps)"),
                          })};
  return schema;
}

int main() {
  printf("helloo\n");
  FLUSH;
  VDP::Schema::PartPtr schema = make_schema();

  std::string schema_str = schema->pretty_print();
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