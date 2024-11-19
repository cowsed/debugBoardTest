#pragma once
#include "vdb/protocol.hpp"

namespace VDP {
class Record : public Part {
  friend PacketReader;
  friend PacketWriter;

public:
  using SizeT = uint32_t;
  explicit Record(std::string name);
  Record(std::string name, const std::vector<Part *> &fields);
  Record(std::string name, std::vector<PartPtr> fields);
  Record(std::string name, PacketReader &reader);
  void setFields(std::vector<PartPtr> fields);

  void fetch() override;
  void read_data_from_message(PacketReader &reader) override;

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
  explicit String(
      std::string name, FetchFunc fetcher = []() { return "no value"; });
  void fetch() override;
  void setValue(std::string new_value);

  void read_data_from_message(PacketReader &reader) override;

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
// Fixed size, numeric types  such as uin8_t, uint32, float, double
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
  explicit Number(
      std::string field_name,
      FetchFunc fetcher = []() { return (NumberType)0; })
      : Part(field_name), fetcher(fetcher) {}

  void fetch() override { value = fetcher(); }
  void setValue(NumberType val) { this->value = val; }

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
  void read_data_from_message(PacketReader &reader) override {
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
  FetchFunc fetcher;
  NumberType value = (NumberType)0;
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
