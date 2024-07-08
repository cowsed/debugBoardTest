#pragma once

#include "vdp.h"
#include "vex.h"

namespace VDP {
class Device {
  Device(int32_t port);

private:
  int32_t port;
};
} // namespace VDP

// // define your global instances of motors and other devices here
// #define MIKE_BUFLEN 32
// #define SERIAL_PORT PORT1
// int main() {
//   static uint8_t buf[MIKE_BUFLEN] = {0};
//   Brain.Screen.printAt(10, 50, "Hello V5");
//   vexGenericSerialEnable(PORT1, 0x0);
//   vexGenericSerialBaudrate(PORT1, 115200);
//   while (1) {
//     int i = vexGenericSerialWriteChar(SERIAL_PORT, 'a');
//     printf("printed : %d\n", i);

//     i = vexGenericSerialReceive(SERIAL_PORT, buf, MIKE_BUFLEN);
//     printf("Recieved: %d at %d\n", i, vexSystemTimeGet());
//     if (i > 0) {
//       printf("Message: %.*s\n", i, buf);
//     }
//     // Allow other tasks to run
//     this_thread::sleep_for(500);
//   }
// }
