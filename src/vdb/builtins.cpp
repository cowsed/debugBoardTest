#include "vdb/builtins.h"

#include "vdb/protocol.h"
#include "vdb/types.h"

#include "vex_motor.h"
#include "vex_units.h"
#include <cstdint>
#include <string>
#include <utility>

namespace VDP {

Motor::Motor(std::string name, vex::motor &mot)
    : Record(std::move(name)), mot(mot), pos(new Float("Position(deg)")),
      vel(new Float("velocity(dps)")), temp(new Uint8("Temperature(C)")),
      voltage(new Float("Voltage(V)")), current(new Float("Current(%)")) {
  Record::setFields({pos, vel, temp, voltage, current});
}

void Motor::fetch() {
  pos->setValue((float)mot.position(vex::rotationUnits::deg));
  vel->setValue((float)mot.velocity(vex::velocityUnits::dps));
  temp->setValue((uint8_t)mot.temperature(vex::temperatureUnits::celsius));
  voltage->setValue((float)mot.voltage(vex::voltageUnits::volt));
  current->setValue((float)mot.current(vex::percentUnits::pct));
}

} // namespace VDP
