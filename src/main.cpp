/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Module:       main.cpp                                                  */
/*    Author:       unknown                                                   */
/*    Created:      7/7/2024, 3:53:01 PM                                      */
/*    Description:  V5 project                                                */
/*                                                                            */
/*----------------------------------------------------------------------------*/
#include "vdb/builtins.hpp"
#include "vdb/protocol.hpp"
#include "vdb/registry.hpp"
#include "vdb/tests.hpp"
#include "vex.h"
#include "wrapper_device.hpp"
#include <thread>
using namespace vex;

// A global instance of vex::brain used for printing to the V5 brain screen
vex::brain Brain;

void print_multiline(const std::string &str, int y, int x);

VDB::Device dev1{vex::PORT1, 115200 * 2};
// VDB::Device dev2{vex::PORT10, 115200 * 8};
VDP::Registry reg1{&dev1, VDP::Registry::Side::Controller};
// VDP::Registry reg2{&dev2, VDP::Registry::Side::Listener};

int main() {

  vex::motor mot1{vex::PORT11};
  printf("opening channel\n");
  auto motorData = (std::shared_ptr<VDP::Timestamped>)new VDP::Timestamped(
      "motor", new VDP::Motor("motor", mot1));

  VDP::ChannelID chan1 = reg1.open_channel(motorData);
  // VDP::ChannelID chan2 = reg1.open_channel(distData);

  bool ready = reg1.negotiate();
  if (!ready) {
    Brain.Screen.printAt(20, 20, "FAILED");
    while (1) {
      vexDelay(1000);
    };
    return 1;
  }
  mot1.spin(vex::fwd, 1, vex::volt);

  while (true) {
    motorData->fetch();
    // distData->fetch();
    reg1.send_data(chan1, motorData);
    // reg1.send_data(chan2, distData);
    vexDelay(100);
  }

  // int count = 0;
  // int sent = 0;
  // reg2.install_data_callback([&](const VDP::Channel &pac) {
  //   print_multiline(pac.data->pretty_print_data(), 1, 1);
  //   count++;
  // });
  // mot1.spin(vex::fwd, 1.0, vex::volt);
  // vex::timer tmr;
  // while (tmr.value() < 5.0) {
  //   motorData->fetch();
  //   bool really = reg1.send_data(chan1, motorData);
  //   if (really) {
  //     sent++;
  //   }
  //   // this_thread::yield();
  //   // vexDelay(100);
  // }
  // mot1.stop();
  // int c2 = count;
  // vexDelay(1000);
  // printf("Got %d decodes with 5 seconds of rapid sending. sent %d\n", c2,
  // sent); printf("%d failed checksum %d too small \n", reg2.num_bad,
  // reg2.num_small);

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

  //   // VDP::PacketReader reader{msg};`
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

void print_multiline(const std::string &str, int y, int x) {
  size_t line_count = 0;
  size_t line_start = 0;
  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == '\n') {
      std::string line = str.substr(line_start, i - line_start);
      Brain.Screen.setCursor(y + (int)line_count, x);
      Brain.Screen.print("%s", line.c_str());
      line_count++;
      line_start = i + 1;
    } else if (str[i] == '\r' || str[i] == '\t') {
    }
  }
}
