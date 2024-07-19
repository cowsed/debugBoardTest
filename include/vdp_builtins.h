#pragma once

#include "vdp.h"
#include "vex.h"

namespace VDP {

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