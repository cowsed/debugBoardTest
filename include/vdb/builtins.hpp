#pragma once

#include "vdb/types.hpp"
#include <memory>

#include "vex.h"
namespace VDP {

class Timestamped : public Record {
public:
  Timestamped(std::string name, Part *data);

  void fetch();

private:
  std::shared_ptr<Uint32> timestamp;
  PartPtr data;
};

class Motor : public Record {
public:
  Motor(std::string name, vex::motor &mot);
  void fetch() override;

private:
  vex::motor &mot;

  std::shared_ptr<Float> pos;
  std::shared_ptr<Float> vel;
  std::shared_ptr<Uint8> temp;
  std::shared_ptr<Float> voltage;
  std::shared_ptr<Float> current;
};
} // namespace VDP
