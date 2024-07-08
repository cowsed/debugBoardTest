#include "vdp.h"
#include <cstring>

namespace VDP {
namespace Schema {

void Part::write_null_terminated(Packet &sofar, const std::string &str) {
  // sofar.
  // for (uint8_t  c : str){

  // }
  sofar.insert(sofar.end(), str.begin(), str.end());

  sofar.push_back(0);
  return;
}

Double::Double(std::string name) : name(name) {}
void Double::add_to_schema(Packet &sofar) const {
  write_null_terminated(sofar, name);
  sofar.push_back((uint8_t)Type::Double);
}

void Double::write_message(Packet &sofar) const {}

} // namespace Schema
} // namespace VDP
