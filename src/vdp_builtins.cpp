#include "vdp_builtins.h"

namespace VDP {
namespace Schema {
Motor::Motor(std::string name, vex::motor &mot)
    : Record(name), mot(mot), pos(new Double("Position(deg)")),
      vel(new Double("velocity(dps)")), temp(new Uint8("Temperature(C)")),
      voltage(new Double("Voltage(V)")), current(new Double("Current(%)")) {
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
