#pragma once
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

class PacketReader {
public:
  PacketReader(Packet pac);
  uint8_t get_byte();
  Type get_type();
  std::string get_string();
  template <typename Number> Number get_number() {
    if (read_head + sizeof(Number) >= pac.size()) {
      printf(
          "Reading a number at position %d would read past buffer of size %d\n",
          read_head, pac.size());
      return 0;
    }
  }

private:
  Packet pac;
  size_t read_head;
};

class PacketWriter {
public:
  void clear();
  void write_byte(uint8_t b);

  void write_type(Type t);
  void write_string(const std::string &str);

  const Packet &get_packet() const;

  template <typename Number> void write_number(Number n) {
    std::array<uint8_t, sizeof(Number)> bs;
    std::memcpy(&bs, &n, sizeof(Number));
    for (uint8_t b : bs) {
      write_byte(b);
    }
  }

private:
  Packet sofar;
};
class Part {
public:
  Part(std::string name);
  std::string pretty_print() const;
  std::string pretty_print_data() const;

  virtual void write_schema(PacketWriter &sofar) const = 0;
  virtual void write_to_message(PacketWriter &sofar) const = 0;
  virtual void read_from_message(PacketReader &reader) = 0;

  virtual void fetch() = 0;
  virtual void pprint(std::stringstream &ss, size_t indent) const = 0;
  virtual void pprint_data(std::stringstream &ss, size_t indent) const = 0;

protected:
  std::string name;
};
using PartPtr = std::shared_ptr<Part>;

class Record : public Part {
public:
  using SizeT = uint32_t;
  Record(std::string name);
  Record(std::string name, std::vector<Part *> fields);
  Record(std::string name, std::vector<PartPtr> fields);
  Record(std::string name, PacketReader &reader);
  void write_schema(PacketWriter &sofar) const override;
  void write_to_message(PacketWriter &sofar) const override;
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
  void write_to_message(PacketWriter &sofar) const override;
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
template <typename Number, Type schemaType> class NumericPart : public Part {
public:
  using NumberType = Number;
  static constexpr Type SchemaType = schemaType;

  // Checks to make sure this isn't misused
  static_assert(std::is_floating_point<Number>::value ||
                    std::is_integral<Number>::value,
                "Number type this is instantiated with must be floating point "
                "or integral");

  using FetchFunc = std::function<Number()>;
  NumericPart(
      std::string name, FetchFunc fetcher = []() { return (Number)0; })
      : Part(name), fetcher(fetcher) {}

  void write_schema(PacketWriter &sofar) const override {
    sofar.write_type(SchemaType); // Type
    sofar.write_string(name);     // Name
  }
  void write_to_message(PacketWriter &sofar) const override {
    sofar.write_number<NumberType>(value);
  }
  void read_from_message(PacketReader &reader) override {
    value = reader.get_number<NumberType>();
  }

  void fetch() override { value = fetcher(); }
  void setValue(Number value) { this->value = value; }

  void pprint(std::stringstream &ss, size_t indent) const override {
    add_indents(ss, indent);
    ss << name << to_string(SchemaType);
  }
  void pprint_data(std::stringstream &ss, size_t indent) const override {
    add_indents(ss, indent);
    ss << name << ":\t" << value;
  }

private:
  Number value = (Number)0;
  FetchFunc fetcher;
};

using Float = NumericPart<float, Type::Float>;
using Double = NumericPart<double, Type::Double>;

using Uint8 = NumericPart<uint8_t, Type::Uint8>;
using Uint16 = NumericPart<uint16_t, Type::Uint16>;
using Uint32 = NumericPart<uint32_t, Type::Uint32>;
using Uint64 = NumericPart<uint64_t, Type::Uint64>;

} // namespace Schema
Schema::PartPtr decode_schema(const Packet &packet);

} // namespace VDP