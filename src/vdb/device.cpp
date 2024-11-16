#include "vdb/device.h"
#include "vdb/protocol.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace VDB {

Device::Device(int32_t port, VDP::Registry &reg)
    : reg(reg), port(port), outbound_packets() {
  vexGenericSerialEnable(port, 0x0);
  vexGenericSerialBaudrate(port, baud_rate);

  hw_task = vex::task(Device::hardware_thread, (void *)this);
  // decode_task = vex::task(Device::decoder_thread, (void *)this);
}

void Device::handle_inbound_byte(uint8_t b) {
  inbound_mutex.lock();

  if (b == 0x00 && inbound_buffer.size() > 0) {
    if (inbound_packets.size() < MAX_IN_QUEUE_SIZE) {
      inbound_packets.push_front(inbound_buffer);
    } else {
      printf("Port%ld: Dropping inbound packet. inbound queue full\n",
             (port + 1));
    }
    // Starting a new packet now
    inbound_buffer.clear();
  } else if (b != 0x00) {
    inbound_buffer.push_back(b);
  }
  inbound_mutex.unlock();
}

int Device::decoder_thread(void *vself) {
  Device &self = *(Device *)vself;
  VDP::Packet decoded = {};

  while (true) {

    self.inbound_mutex.lock();

    WirePacket inbound = {};
    if (self.inbound_packets.size() > 0) {
      inbound = self.inbound_packets.back();
      self.inbound_packets.pop_back();
    }
    self.inbound_mutex.unlock();

    if (inbound.size() == 0) {
      // no packet read, wait a bit then see if we have anything to do
      vexDelay(1);
      continue;
    }

    CobsDecode(inbound, decoded);
    self.reg.take_packet(decoded);

    // give up control for a bit
    vex::this_thread::yield();
  }
  return 0;
}

bool Device::write_packet() {
  bool did_write = false;
  WirePacket outbound_packet = {};
  outbound_mutex.lock();
  if (outbound_packets.size() > 0) {
    outbound_packet = outbound_packets.back();
    outbound_packets.pop_back();
  }
  outbound_mutex.unlock();
  if (outbound_packet.size() > 0) {
    const int avail = vexGenericSerialWriteFree(port);
    if (avail < (int)outbound_packet.size()) {
      // avail can be -1 for unknown reasons.
      // Signed comparison handles this correctly (as
      // correctly as I can think to).
      vexGenericSerialFlush(port);
    }
    const size_t wrote = vexGenericSerialTransmit(
        port, outbound_packet.data(), (int32_t)outbound_packet.size());
    if (wrote < outbound_packet.size()) {
      printf("Unhandled state: Didn't write all that we wanted to, will "
             "probably get packet corruption\n");
    } else {
      printf("Wrote %d bytes\n", wrote);
    }
    did_write = true;
  }
  return did_write;
}

int Device::hardware_thread(void *vself) {
  Device &self = *(Device *)vself;

  static constexpr size_t buflen = 32;
  static uint8_t buf[buflen] = {0};
  printf("HERERE\n");
  while (1) {
    // Lame replacement for blocking IO. We can't just wait and tell the
    // scheduler to go work on something else while we wait for packets so
    // instead, if we're getting nothing in and have nothing to send, block
    // ourselves.
    bool did_something = false;

    // Writing
    did_something |= self.write_packet();
    // Reading
    // const int avail = vexGenericSerialReceiveAvail(self.port);
    // if (avail > 0) {
    // const int read = vexGenericSerialReceive(self.port, buf, buflen);
    // for (int i = 0; i < read; i++) {
    // self.handle_inbound_byte(buf[i]);
    // }
    // did_something = true;
    // }

    if (!did_something) {
      // dont consume all the resources if theres nothing to do
      vexDelay(1);
    }
  }
  return 0;
}

void Device::send_packet(const VDP::Packet &pac) {
  std::vector<uint8_t> encoded;
  CobsEncode(pac, encoded);

  outbound_mutex.lock();
  outbound_packets.push_back(encoded);
  outbound_mutex.unlock();
}

void Device::CobsEncode(const VDP::Packet &in, WirePacket &out) {
  out.clear();
  if (in.size() == 0) {
    return;
  }
  size_t output_size = in.size() + (in.size() / 254) + 1;
  output_size += 2; // delimeter bytes
  out.resize(output_size);
  out[0] = 0;

  size_t output_code_head = 1;
  size_t code_value = 1;

  size_t input_head = 0;
  size_t output_head = 2;
  size_t length = 0;
  while (input_head < in.size()) {
    if (in[input_head] == 0 || length == 255) {
      out[output_code_head] = code_value;
      code_value = 1;
      output_code_head = output_head;
      length = 0;
      output_head++;
      if (in[input_head] == 0) {
        input_head++;
      }
    } else {
      out[output_head] = in[input_head];
      code_value++;
      input_head++;
      output_head++;
    }
    length++;
  }
  out[output_code_head] = code_value;

  // Trailing delimeter
  out[output_head] = 0;
  output_head++;

  out.resize(output_head);
}

void Device::CobsDecode(const WirePacket &in, VDP::Packet &out) {
  out.clear();
  if (in.size() == 0) {
    return;
  }

  out.resize(in.size() + in.size() / 254);
  uint8_t code = 0xff;
  uint8_t left_in_block = 0;
  size_t write_head = 0;
  for (const uint8_t byte : in) {
    if (left_in_block) {
      out[write_head] = byte;
      write_head++;
    } else {
      left_in_block = byte;
      if (left_in_block != 0 && (code != 0xff)) {
        out[write_head] = 0;
        write_head++;
      }
      code = left_in_block;
      if (code == 0) {
        // hit a delimeter
        break;
      }
    }
    left_in_block--;
  }
  out.resize(write_head);

  return;
}

// Dummy child of vexlink to get access to their crc32 checksum calculation
// function
class dummy_vexlink : vex::vexlink {
public:
  static unsigned int crc32_one(unsigned int a, unsigned char c) {
    return vexlink::crc32(&c, 1, a);
  }
  static unsigned int crc32_buf(unsigned int a, const unsigned char *c,
                                uint32_t len) {
    // const cast :skull:
    return vexlink::crc32(const_cast<uint8_t *>(c), len, a);
  }
};

} // namespace VDB

namespace VDP {

uint32_t crc32_one(uint32_t accum, uint8_t b) {
  return VDB::dummy_vexlink::crc32_one(accum, b);
}
uint32_t crc32_buf(uint32_t accum, const uint8_t *b, uint32_t length) {
  return VDB::dummy_vexlink::crc32_buf(accum, b, length);
}
} // namespace VDP
