#pragma once
#include <string>
#include <vector>
namespace VDP {
using Packet = std::vector<uint8_t>;

namespace Schema {
enum class Type : uint8_t {
  Double = 0,
  String = 1,
};

class Part {
public:
  virtual void add_to_schema(Packet &sofar) const = 0;
  virtual void write_message(Packet &sofar) const = 0;
  static void write_null_terminated(Packet &sofra, const std::string &str);
};
class Double : public Part {
public:
  Double(std::string name);
  void add_to_schema(Packet &sofar) const override;
  void write_message(Packet &sofar) const override;

private:
  std::string name;
};

class String : public Part {
  String(std::string name);

private:
  std::string name;
};

} // namespace Schema
} // namespace VDP