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
using Packet = std::vector<uint8_t>;
void dump_packet(const Packet &pac);

namespace Schema {
enum class Type : uint8_t {
  Record,
  String,
  // Enum

  Double,
  Float,

  Uint8,
  Uint16,
  Uint32,
  Uint64,

  Int8,
  Int16,
  Int32,
  Int64,

};
std::string to_string(Type t);
void add_indents(std::stringstream &ss, size_t indent);

class PacketReader;
class PacketWriter;

class Part {
public:
  Part(std::string name);
  std::string pretty_print() const;
  std::string pretty_print_data() const;

  virtual void write_schema(PacketWriter &sofar) const = 0;
  virtual void write_message(PacketWriter &sofar) const = 0;
  virtual void read_from_message(PacketReader &reader) = 0;

  virtual void fetch() = 0;
  virtual void pprint(std::stringstream &ss, size_t indent) const = 0;
  virtual void pprint_data(std::stringstream &ss, size_t indent) const = 0;

protected:
  std::string name;
};
using PartPtr = std::shared_ptr<Part>;

class PacketReader {
public:
  PacketReader(Packet pac);
  uint8_t get_byte();
  Type get_type();
  std::string get_string();

  template <typename Number> Number get_number() {
    static_assert(std::is_floating_point<Number>::value ||
                      std::is_integral<Number>::value,
                  "This function should only be used on numbers");

    printf("Reading number size %d at position %d of %d\n", sizeof(Number),
           read_head, pac.size());
    fflush(stdout);
    vexDelay(400);

    if (read_head + sizeof(Number) >= pac.size()) {
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

  void write_schema(PartPtr part);
  void write_message(PartPtr part);

  const Packet &get_packet() const;

  template <typename Number> void write_number(Number num) {
    std::array<uint8_t, sizeof(Number)> bytes;
    std::memcpy(&bytes, &num, sizeof(Number));
    for (uint8_t b : bytes) {
      write_byte(b);
    }
  }

private:
  Packet sofar;
};

class Record : public Part {
public:
  using SizeT = uint32_t;
  Record(std::string name);
  Record(std::string name, std::vector<Part *> fields);
  Record(std::string name, std::vector<PartPtr> fields);
  Record(std::string name, PacketReader &reader);
  // Encode the schema itself for transmission on the wire
  void write_schema(PacketWriter &sofar) const override;
  // Encode the data currently held according to schema for transmission on the
  // wire
  void write_message(PacketWriter &sofar) const override;

  void read_from_message(PacketReader &reader) override;

  void fetch() override;
  void setFields(std::vector<PartPtr> fields);

  void pprint(std::stringstream &ss, size_t indent) const override;
  void pprint_data(std::stringstream &ss, size_t indent) const override;

private:
  std::vector<PartPtr> fields;
};

class String : public Part {
public:
  using FetchFunc = std::function<std::string()>;
  String(
      std::string name, FetchFunc fetcher = []() { return "no value"; });
  void write_schema(PacketWriter &sofar) const override;
  void write_message(PacketWriter &sofar) const override;
  void read_from_message(PacketReader &reader) override;
  void fetch() override;
  void setValue(std::string new_value);

  void pprint(std::stringstream &ss, size_t indent) const override;
  void pprint_data(std::stringstream &ss, size_t indent) const override;

private:
  FetchFunc fetcher;
  std::string value;
};

// Template to reduce boiler plate for Schema wrappers for simple types
// Basically fixed size, numeric types  such as uin8_t, uint32, float, double
template <typename NumT, Type schemaType> class Number : public Part {
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
      : Part(name), fetcher(fetcher) {
    std::string s = to_string(SchemaType);
    printf("Constructed Number %s\n", s.c_str());
    fflush(stdout);
    vexDelay(400);
  }

  void write_schema(PacketWriter &sofar) const override {
    sofar.write_type(SchemaType); // Type
    sofar.write_string(name);     // Name
  }
  void write_message(PacketWriter &sofar) const override {
    sofar.write_number<NumberType>(value);
  }
  void read_from_message(PacketReader &reader) override {
    value = reader.get_number<NumberType>();
  }

  void fetch() override { value = fetcher(); }
  void setValue(NumberType value) { this->value = value; }

  void pprint(std::stringstream &ss, size_t indent) const override {
    add_indents(ss, indent);
    ss << name << ":\t" << to_string(SchemaType);
  }
  void pprint_data(std::stringstream &ss, size_t indent) const override {
    add_indents(ss, indent);
    ss << name << ":\t" << value;
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

} // namespace Schema
Schema::PartPtr decode_schema(const Packet &packet);

} // namespace VDP