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
  uint8_t val = get_byte();
  return (Type)val;
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
  printf("read string: %s\n", s.c_str());
  return s;
}
void PacketWriter::write_byte(uint8_t b) { sofar.push_back(b); }

void PacketWriter::write_type(Type t) { write_byte((uint8_t)t); }
void PacketWriter::write_string(const std::string &str) {
  sofar.insert(sofar.end(), str.begin(), str.end());
  sofar.push_back(0);
}
void PacketWriter::clear() { sofar.clear(); }
size_t PacketWriter::size() { return sofar.size(); }

const Packet &PacketWriter::get_packet() const { return sofar; }

void PacketWriter::write_schema(PartPtr part) {
  clear();
  part->write_schema(*this);
}
void PacketWriter::write_message(PartPtr part) {
  clear();
  part->write_message(*this);
}

std::string to_string(Type t) {
  switch (t) {
  case Type::Record:
    return "record";
  case Type::String:
    return "string";
  case Type::Float:
    return "double";
  case Type::Double:
    return "double";

  case Type::Uint8:
    return "uint8";
  case Type::Uint16:
    return "uint16";
  case Type::Uint32:
    return "uint32";
  case Type::Uint64:
    return "uint64";

  case Type::Int8:
    return "int8";
  case Type::Int16:
    return "int16";
  case Type::Int32:
    return "int32";
  case Type::Int64:
    return "int64";
  }

  return "<<UNKNOWN TYPE>>";
}
void add_indents(std::stringstream &ss, size_t indent) {
  for (size_t i = 0; i < indent; i++) {
    ss << "  ";
  }
}

PartPtr make_decoder(PacketReader &pac) {
  printf("make_decoder entered\n");
  Type t = pac.get_type();
  std::string tname = to_string(t);
  std::string name = pac.get_string();
  printf("Making decoder for '%s' of type %s\n", name.c_str(), tname.c_str());
  fflush(stdout);
  vexDelay(400);

  switch (t) {
  case Type::String:
    return PartPtr(new String(name));
  case Type::Record:
    return PartPtr(new Record(name, pac));

  case Type::Float:
    return PartPtr(new Float(name));
  case Type::Double:
    return PartPtr(new Double(name));

  case Type::Uint8:
    return PartPtr(new Uint8(name));
  case Type::Uint16:
    return PartPtr(new Uint16(name));
  case Type::Uint32:
    return PartPtr(new Uint32(name));
  case Type::Uint64:
    return PartPtr(new Uint64(name));

  case Type::Int8:
    return PartPtr(new Int8(name));
  case Type::Int16:
    return PartPtr(new Int16(name));
  case Type::Int32:
    return PartPtr(new Int32(name));
  case Type::Int64:
    return PartPtr(new Int64(name));
  }
  return nullptr;
}

Part::Part(std::string name) : name(name) {}

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
  // Name and type already read, only need to read number of fields before child
  // data shows up
  uint32_t size = reader.get_number<SizeT>();
  printf("Found %lu fields\n", size);
  fflush(stdout);
  vexDelay(400);
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
void Record::write_schema(PacketWriter &sofar) const {
  sofar.write_type(Type::Record);           // Type
  sofar.write_string(name);                 // Name
  sofar.write_number<SizeT>(fields.size()); // Number of fields
  for (const PartPtr &field : fields) {
    field->write_schema(sofar);
  }
}
void Record::write_message(PacketWriter &sofar) const {
  printf("Writer size: %d\n", sofar.size());
  for (auto f : fields) {
    f->write_message(sofar);
  }
}

void Record::read_from_message(PacketReader &reader) {
  for (auto f : fields) {
    f->read_from_message(reader);
  }
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

void String::write_schema(PacketWriter &sofar) const {
  sofar.write_type(Type::String); // Type
  sofar.write_string(name);       // Name
}

void String::write_message(PacketWriter &sofar) const {
  sofar.write_string(value);
}

void String::read_from_message(PacketReader &reader) {
  value = reader.get_string();
}
} // namespace Schema

Schema::PartPtr decode_schema(const Packet &packet) {
  printf("Decode schema\n");
  fflush(stdout);
  vexDelay(400);

  Schema::PacketReader reader(packet);
  return make_decoder(reader);
}

} // namespace VDP
