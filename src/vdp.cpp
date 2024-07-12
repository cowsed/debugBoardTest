#include "vdp.h"
#include "vex.h"
#include <cstring>
#include <stdio.h>

#define TODO()                                                                 \
  printf("UNIMPLEMENTED %s at %s:%d\n", __func__, __FILE__, __LINE__);

namespace VDP {
void dump_packet(const Packet &pac) {
  int i = 0;
  for (const uint8_t d : pac) {
    if (i % 16 == 0 && i != 0) {
      printf("\n");
    }
    printf("%02x ", (int)d);
    i++;
  }
  printf("\n");
}
namespace Schema {

std::string Part::pretty_print() const {
  std::stringstream ss;
  this->pprint(ss, 0);
  return ss.str();
}
std::string Part::pretty_print_data() const {
  std::stringstream ss;
  this->pprint_data(ss, 0);
  return ss.str();
}

PacketReader::PacketReader(Packet pac) : pac(pac), read_head(0) {}

uint8_t PacketReader::get_byte() {
  uint8_t b = pac[read_head];
  read_head++;
  return b;
}

Type PacketReader::get_type() {
  uint8_t val = pac[read_head];
  read_head++;
  return (Type)val;
}

uint32_t PacketReader::get_uint32() {
  uint32_t val = 0;
  for (size_t b = 0; b < sizeof(val); b++) {
    uint8_t byte = get_byte();
    val |= byte << (8 * b);
  }
  return val;
}

double PacketReader::get_double() {
  double val = 0;
  uint8_t data[sizeof(val)];
  for (size_t b = 0; b < sizeof(val); b++) {
    uint8_t byte = get_byte();
    data[b] = byte;
  }
  std::memcpy(&val, data, sizeof(val));
  return val;
}

std::string PacketReader::get_string() {
  std::string s;

  while (1) {
    uint8_t c = get_byte();
    if (c == 0) {
      break;
    }
    s.push_back(c);
  }
  return s;
}

std::string to_string(Type t) {
  switch (t) {
  case Type::Double:
    return "double";
  case Type::String:
    return "string";
  case Type::Record:
    return "record";
  default:
    return "<<UNKNOWN TYPE>>";
  }
}
void add_indents(std::stringstream &ss, size_t indent) {
  for (size_t i = 0; i < indent; i++) {
    ss << "  ";
  }
}

PartPtr make_decoder(PacketReader &pac) {
  Type t = pac.get_type();
  std::string name = pac.get_string();
  switch (t) {
  case Type::Double:
    return PartPtr(new Double(name));
  case Type::String:
    return PartPtr(new String(name));
  case Type::Record:
    return PartPtr(new Record(name, pac));
  default:
    return nullptr;
    break;
  }
}

Part::Part(std::string name) : name(name) {}

void Part::write_null_terminated(Packet &sofar, const std::string &str) {
  sofar.insert(sofar.end(), str.begin(), str.end());
  sofar.push_back(0);
  return;
}
void Part::write_uint32(Packet &sofar, uint32_t val) {
  for (size_t b = 0; b < sizeof(val); b++) {
    sofar.push_back((val >> (8 * b)) & 0xFF);
  }
}

void Part::write_double(Packet &sofar, double val) {
  std::array<uint8_t, sizeof(val)> bytes;
  std::memcpy(bytes.data(), &val, sizeof(val));
  for (uint8_t b : bytes) {
    sofar.push_back(b);
  }
}

Record::Record(std::string name, std::vector<Part *> parts)
    : Part(name), fields() {
  fields.reserve(parts.size());
  for (Part *f : parts) {
    fields.emplace_back(f);
  }
}
Record::Record(std::string name) : Part(name), fields({}) {}
void Record::setFields(std::vector<PartPtr> fs) { fields = fs; }

Record::Record(std::string name, std::vector<PartPtr> parts)
    : Part(name), fields(parts) {}

Record::Record(std::string name, PacketReader &reader) : Part(name), fields() {
  uint32_t size = reader.get_uint32();
  fields.reserve(size);
  for (size_t i = 0; i < size; i++) {
    fields.push_back(make_decoder(reader));
  }
}
void Record::fetch() {
  for (auto &field : fields) {
    field->fetch();
  }
}
void Record::write_to_schema(Packet &sofar) const {
  sofar.push_back((uint8_t)Type::Record); // Type
  write_null_terminated(sofar, name);     // Name
  write_uint32(sofar, fields.size());     // Number of fields
  for (const PartPtr &field : fields) {
    field->write_to_schema(sofar);
  }
}
void Record::write_to_message(Packet &sofar) const {
  for (auto f : fields) {
    f->write_to_message(sofar);
  }
}

void Record::read_from_message(PacketReader &reader) {
  for (auto f : fields) {
    f->read_from_message(reader);
  }
}

Double::Double(std::string name, FetchFunc fetcher)
    : Part(name), fetcher(fetcher) {}

void Double::fetch() { value = fetcher(); }

void Double::setValue(double new_value) { value = new_value; }

void Double::write_to_schema(Packet &sofar) const {
  sofar.push_back((uint8_t)Type::Double); // Type
  write_null_terminated(sofar, name);     // Name
}

void Double::write_to_message(Packet &sofar) const {
  write_double(sofar, value);
}
void Double::read_from_message(PacketReader &reader) {
  value = reader.get_double();
}

void Double::pprint(std::stringstream &ss, size_t indent) const {
  add_indents(ss, indent);
  ss << name << ": double";
}
void Double::pprint_data(std::stringstream &ss, size_t indent) const {
  add_indents(ss, indent);
  ss << name << ":\t" << value;
}

void String::pprint(std::stringstream &ss, size_t indent) const {
  add_indents(ss, indent);
  ss << name << ": string";
}
void String::pprint_data(std::stringstream &ss, size_t indent) const {
  add_indents(ss, indent);
  ss << name << ":\t" << value;
}

void Record::pprint(std::stringstream &ss, size_t indent) const {
  add_indents(ss, indent);
  ss << name << ": record[" << fields.size() << "]{\n";
  for (const auto &f : fields) {

    f->pprint(ss, indent + 1);
    ss << '\n';
  }
  add_indents(ss, indent);
  ss << "}\n";
}
void Record::pprint_data(std::stringstream &ss, size_t indent) const {
  add_indents(ss, indent);
  ss << name << ": record[" << fields.size() << "]{\n";
  for (const auto &f : fields) {

    f->pprint_data(ss, indent + 1);
    ss << '\n';
  }
  add_indents(ss, indent);
  ss << "}\n";
}

String::String(std::string name, std::function<std::string()> fetcher)
    : Part(name), fetcher(fetcher) {}

void String::fetch() { value = fetcher(); }

void String::setValue(std::string new_value) { value = new_value; }

void String::write_to_schema(Packet &sofar) const {
  sofar.push_back((uint8_t)Type::String); // Type
  write_null_terminated(sofar, name);     // Name
}

void String::write_to_message(Packet &sofar) const {
  write_null_terminated(sofar, value);
}

void String::read_from_message(PacketReader &reader) {
  value = reader.get_string();
}
} // namespace Schema

Schema::PartPtr decode_schema(Packet &&packet) {
  Schema::PacketReader reader(packet);
  return make_decoder(reader);
}

} // namespace VDP
