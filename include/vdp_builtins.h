#pragma once

#include "vdp.h"
#include "vex.h"

namespace VDP {
namespace Schema {

class Motor : public Record {
public:
  Motor(std::string name, vex::motor &mot);
  void fetch() override;

private:
  vex::motor &mot;

  std::shared_ptr<Double> pos;
  std::shared_ptr<Double> vel;
  std::shared_ptr<Uint8> temp;
  std::shared_ptr<Double> voltage;
  std::shared_ptr<Double> current;
};
} // namespace Schema
} // namespace VDP