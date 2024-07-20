#pragma once
#include "vex.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace VDP {
class Part;
using PartPtr = std::shared_ptr<Part>;

using ChannelID = uint8_t;
struct Channel {
  ChannelID id;
  PartPtr data;
};
using Packet = std::vector<uint8_t>;
void dump_packet(const Packet &pac);
Channel decode_broadcast(const Packet &packet);

enum class PacketValidity : uint8_t {
  Ok,
  BadChecksum,
  TooSmall,
};

enum class PacketType : uint8_t {
  Broadcast = 0,
  Data = 1,
};
enum class PacketFunction : uint8_t {
  Send = 0,
  Acknowledge = 1,
};
struct PacketHeader {
  PacketType type;
  PacketFunction func;
};

uint8_t make_header_byte(PacketHeader head);
PacketHeader decode_header_byte(uint8_t hb);

enum class Type : uint8_t {
  Record = 0,
  String = 1,
  // Enum

  Double = 3,
  Float = 4,

  Uint8 = 5,
  Uint16 = 6,
  Uint32 = 7,
  Uint64 = 8,

  Int8 = 9,
  Int16 = 10,
  Int32 = 11,
  Int64 = 12,

};
std::string to_string(Type t);
void add_indents(std::stringstream &ss, size_t indent);

class PacketReader;
class PacketWriter;

class Part {
  friend class PacketReader;
  friend class PacketWriter;
  friend class Record;

public:
  Part(std::string name);
  virtual ~Part() {}
  std::string pretty_print() const;
  std::string pretty_print_data() const;

  virtual void fetch() = 0;
  virtual void read_from_message(PacketReader &reader) = 0;

protected:
  // These are needed to decode correctly but you shouldn't call them directly
  virtual void write_schema(PacketWriter &sofar) const = 0;
  virtual void write_message(PacketWriter &sofar) const = 0;

  virtual void pprint(std::stringstream &ss, size_t indent) const = 0;
  virtual void pprint_data(std::stringstream &ss, size_t indent) const = 0;

  std::string name;
};

class PacketReader {
public:
  PacketReader(Packet pac);
  uint8_t get_byte();
  Type get_type();
  std::string get_string();
  uint32_t calc_crc32();

  template <typename Number> Number get_number() {
    static_assert(std::is_floating_point<Number>::value ||
                      std::is_integral<Number>::value,
                  "This function should only be used on numbers");

    if (read_head + sizeof(Number) > pac.size()) {
      printf("Reading a number[%d] at position %d would read past buffer of "
             "size %d\n",
             sizeof(Number), read_head, pac.size());
      return 0;
    }
    Number value = 0;
    std::memcpy(&value, &pac[read_head], sizeof(Number));
    read_head += sizeof(Number);
    return value;
  }

private:
  Packet pac;
  size_t read_head;
};

class PacketWriter {
public:
  void clear();
  size_t size();
  void write_byte(uint8_t b);

  void write_type(Type t);
  void write_string(const std::string &str);

  void write_channel_broadcast(const Channel &chan);
  void write_message(const Channel &part);

  const Packet &get_packet() const;

  template <typename Number> void write_number(Number num) {
    std::array<uint8_t, sizeof(Number)> bytes;
    std::memcpy(&bytes, &num, sizeof(Number));
    for (const uint8_t b : bytes) {
      write_byte(b);
    }
  }

private:
  Packet sofar;
};

class Record : public Part {
  friend PacketReader;
  friend PacketWriter;

public:
  using SizeT = uint32_t;
  Record(std::string name);
  Record(std::string name, const std::vector<Part *> &fields);
  Record(std::string name, std::vector<PartPtr> fields);
  Record(std::string name, PacketReader &reader);
  void setFields(std::vector<PartPtr> fields);

  void fetch() override;
  void read_from_message(PacketReader &reader) override;

protected:
  // Encode the schema itself for transmission on the wire
  void write_schema(PacketWriter &sofar) const override;
  // Encode the data currently held according to schema for transmission on the
  // wire
  void write_message(PacketWriter &sofar) const override;

private:
  void pprint(std::stringstream &ss, size_t indent) const override;
  void pprint_data(std::stringstream &ss, size_t indent) const override;

  std::vector<PartPtr> fields;
};

class String : public Part {
  friend PacketReader;
  friend PacketWriter;

public:
  using FetchFunc = std::function<std::string()>;
  String(std::string name, FetchFunc fetcher = []() { return "no value"; });
  void fetch() override;
  void setValue(std::string new_value);

  void read_from_message(PacketReader &reader) override;

  void pprint(std::stringstream &ss, size_t indent) const override;
  void pprint_data(std::stringstream &ss, size_t indent) const override;

protected:
  void write_schema(PacketWriter &sofar) const override;
  void write_message(PacketWriter &sofar) const override;

private:
  FetchFunc fetcher;
  std::string value;
};

// Template to reduce boiler plate for Schema wrappers for simple types
// Basically fixed size, numeric types  such as uin8_t, uint32, float, double
template <typename NumT, Type schemaType> class Number : public Part {
  friend PacketReader;
  friend PacketWriter;

public:
  using NumberType = NumT;
  static constexpr Type SchemaType = schemaType;

  // Checks to make sure this isn't misused
  static_assert(std::is_floating_point<NumberType>::value ||
                    std::is_integral<NumberType>::value,
                "Number type this is instantiated with must be floating point "
                "or integral");

  using FetchFunc = std::function<NumberType()>;
  Number(
      std::string name, FetchFunc fetcher = []() { return (NumberType)0; })
      : Part(name), fetcher(fetcher) {}

  void fetch() override { value = fetcher(); }
  void setValue(NumberType value) { this->value = value; }

  void pprint(std::stringstream &ss, size_t indent) const override {
    add_indents(ss, indent);
    ss << name << ":\t" << to_string(SchemaType);
  }
  void pprint_data(std::stringstream &ss, size_t indent) const override {
    add_indents(ss, indent);
    ss << name << ":\t";
    if (sizeof(NumberType) == 1) {
      ss << (int)value; // Otherwise, stringstream interprets uint8 as char and
                        // prints a char
    } else {
      ss << value;
    }
  }
  void read_from_message(PacketReader &reader) override {
    value = reader.get_number<NumberType>();
  }

protected:
  void write_schema(PacketWriter &sofar) const override {
    sofar.write_type(SchemaType); // Type
    sofar.write_string(name);     // Name
  }
  void write_message(PacketWriter &sofar) const override {
    sofar.write_number<NumberType>(value);
  }

private:
  NumberType value = (NumberType)0;
  FetchFunc fetcher;
};

using Float = Number<float, Type::Float>;
using Double = Number<double, Type::Double>;

using Uint8 = Number<uint8_t, Type::Uint8>;
using Uint16 = Number<uint16_t, Type::Uint16>;
using Uint32 = Number<uint32_t, Type::Uint32>;
using Uint64 = Number<uint64_t, Type::Uint64>;

using Int8 = Number<int8_t, Type::Int8>;
using Int16 = Number<int16_t, Type::Int16>;
using Int32 = Number<int32_t, Type::Int32>;
using Int64 = Number<int64_t, Type::Int64>;

} // namespace VDP