#pragma once
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace VDP {
using Packet = std::vector<uint8_t>;
void dump_packet(const Packet &pac);

namespace Schema {
enum class Type : uint8_t {
  Double = 0,
  String = 1,
  Record = 2,
};
std::string to_string(Type t);

class PacketReader {
public:
  PacketReader(Packet pac);
  uint8_t get_byte();
  Type get_type();
  uint32_t read_uint32();
  std::string get_string();

private:
  Packet pac;
  size_t read_head;
};

class Part {
public:
  Part(std::string name);
  std::string pretty_print() const;

  static void write_null_terminated(Packet &sofra, const std::string &str);
  static void write_uint32(Packet &sofar, uint32_t val);
  static void write_double(Packet &sofar, double val);

  virtual void write_to_schema(Packet &sofar) const = 0;
  virtual void write_message(Packet &sofar) const = 0;

  virtual void fetch() = 0;
  virtual void pprint(std::stringstream &ss, size_t indent) const = 0;

protected:
  std::string name;
};
using PartPtr = std::shared_ptr<Part>;

class Record : public Part {
public:
  Record(std::string name);
  Record(std::string name, std::vector<Part *> fields);
  Record(std::string name, std::vector<PartPtr> fields);
  Record(std::string name, PacketReader &reader);
  void write_to_schema(Packet &sofar) const override;
  void write_message(Packet &sofar) const override;
  void fetch() override;
  void setFields(std::vector<PartPtr> fields);

  void pprint(std::stringstream &ss, size_t indent) const override;

private:
  std::vector<PartPtr> fields;
};

class Double : public Part {
public:
  using FetchFunc = std::function<double()>;
  Double(
      std::string name, FetchFunc fetcher = []() { return 0; });
  void write_to_schema(Packet &sofar) const override;
  void write_message(Packet &sofar) const override;
  void fetch() override;
  void setValue(double new_value);

  void pprint(std::stringstream &ss, size_t indent) const override;

private:
  FetchFunc fetcher;
  double value;
};

class String : public Part {
public:
  using FetchFunc = std::function<std::string()>;
  String(
      std::string name, FetchFunc fetcher = []() { return "no value"; });
  void write_to_schema(Packet &sofar) const override;
  void write_message(Packet &sofar) const override;
  void fetch() override;
  void setValue(std::string new_value);

  void pprint(std::stringstream &ss, size_t indent) const override;

private:
  FetchFunc fetcher;
  std::string value;
};

} // namespace Schema
Schema::PartPtr decode_schema(Packet &&packet);

} // namespace VDP