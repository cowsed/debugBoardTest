#include "vdp_builtins.h"

namespace VDP {
namespace Schema {
Motor::Motor(std::string name, vex::motor &mot)
    : Record(name), mot(mot), pos(new Float("Position(deg)")),
      vel(new Float("velocity(dps)")), temp(new Uint8("Temperature(C)")),
      voltage(new Float("Voltage(V)")), current(new Float("Current(%)")) {
  Record::setFields({pos, vel, temp, voltage, current});
}

void Motor::fetch() {
  pos->setValue(mot.position(vex::rotationUnits::deg));
  vel->setValue(mot.velocity(vex::velocityUnits::dps));
  temp->setValue((uint8_t)mot.temperature(vex::temperatureUnits::celsius));
  voltage->setValue(mot.voltage(vex::voltageUnits::volt));
  current->setValue(mot.current(vex::percentUnits::pct));
}

} // namespace Schema

} // namespace VDP
