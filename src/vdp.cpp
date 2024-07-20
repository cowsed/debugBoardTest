#include "vdp.h"

#include <cstdint>
#include <cstring>
#include <functional>
#include <sstream>
#include <stdio.h>
#include <string>
#include <utility>
#include <vector>

#include "vex.h"
#include "vex_vexlink.h"

#include "vdp_device.h"

#define TODO()                                                                 \
  printf("UNIMPLEMENTED %s at %s:%d\n", __PRETTY_FUNCTION__, __FILE__,         \
         __LINE__);

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

PacketReader::PacketReader(Packet pac) : pac(std::move(pac)), read_head(0) {}

uint8_t PacketReader::get_byte() {
  const uint8_t b = pac[read_head];
  read_head++;
  return b;
}

Type PacketReader::get_type() {
  const uint8_t val = get_byte();
  return (Type)val;
}

std::string PacketReader::get_string() {
  std::string s;

  while (1) {
    const uint8_t c = get_byte();
    if (c == 0) {
      break;
    }
    s.push_back((char)c);
  }
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

void PacketWriter::write_channel_broadcast(const Channel &chan) {
  clear();
  const uint8_t header = make_header_byte(
      PacketHeader{PacketType::Broadcast, PacketFunction::Send});
  // Header
  write_number<uint8_t>(header);
  write_number<ChannelID>(chan.id);

  // Data
  chan.data->write_schema(*this);

  // Checksum
  auto crc = VDB::crc32_buf(0, sofar.data(), sofar.size());
  write_number<uint32_t>(crc);
}
void PacketWriter::write_message(const Channel &chan) {
  clear();
  const uint8_t header =
      make_header_byte(PacketHeader{PacketType::Data, PacketFunction::Send});
  // Header
  write_number<uint8_t>(header);
  write_number<ChannelID>(chan.id);
  // Data
  chan.data->write_message(*this);
  // Checksum
  auto crc = VDB::crc32_buf(0, sofar.data(), sofar.size());
  write_number<uint32_t>(crc);
}

std::string to_string(Type t) {
  switch (t) {
  case Type::Record:
    return "record";
  case Type::String:
    return "string";

  case Type::Float:
    return "float";
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
  const Type t = pac.get_type();
  const std::string name = pac.get_string();

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

Part::Part(std::string name) : name(std::move(name)) {}

Record::Record(std::string name, const std::vector<Part *> &parts)
    : Part(std::move(name)), fields() {
  fields.reserve(parts.size());
  for (Part *f : parts) {
    fields.emplace_back(f);
  }
}
Record::Record(std::string name) : Part(std::move(name)), fields({}) {}
void Record::setFields(std::vector<PartPtr> fs) { fields = std::move(fs); }

Record::Record(std::string name, std::vector<PartPtr> parts)
    : Part(std::move(name)), fields(std::move(parts)) {}

Record::Record(std::string name, PacketReader &reader)
    : Part(std::move(name)), fields() {
  // Name and type already read, only need to read number of fields before child
  // data shows up
  const uint32_t size = reader.get_number<SizeT>();
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
  for (auto &f : fields) {
    f->write_message(sofar);
  }
}

void Record::read_from_message(PacketReader &reader) {
  for (auto &f : fields) {
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
    : Part(std::move(name)), fetcher(std::move(fetcher)) {}

void String::fetch() { value = fetcher(); }

void String::setValue(std::string new_value) { value = std::move(new_value); }

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
static constexpr auto PACKET_TYPE_BIT_LOCATION = 7;
static constexpr auto PACKET_FUNCTION_BIT_LOCATION = 6;

uint8_t make_header_byte(PacketHeader head) {

  uint8_t b = 0;
  b |= ((uint8_t)head.type) << PACKET_TYPE_BIT_LOCATION;
  b |= ((uint8_t)head.func) << PACKET_FUNCTION_BIT_LOCATION;
  return b;
}
PacketHeader decode_header_byte(uint8_t hb) {
  const PacketType pt = (PacketType)((hb >> PACKET_TYPE_BIT_LOCATION) & 1);
  const PacketFunction func =
      (PacketFunction)((hb >> PACKET_FUNCTION_BIT_LOCATION) & 1);

  return {pt, func};
}

Channel decode_broadcast(const Packet &packet) {

  PacketReader reader(packet);
  // header byte, had to be read to know were a braodcast
  (void)reader.get_byte();
  const ChannelID id = reader.get_number<ChannelID>();
  const PartPtr schema = make_decoder(reader);
  return {id, schema};
}

} // namespace VDP
