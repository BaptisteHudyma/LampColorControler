#include "charger.h"

#include <cstdint>

#include "../alerts.h"
#include "../physical/BQ25703A.h"
#include "../physical/battery.h"
#include "Arduino.h"
#include "FUSB302/PD_UFP.h"

namespace charger {

// Initialise the device and library
bq2573a::BQ25703A charger;

// Create instance of registers data structure
bq2573a::BQ25703A::Regt BQ25703Areg;

PD_UFP_c PD_UFP;

bool check_vendor_device_values() {
  byte manufacturerId = BQ25703Areg.manufacturerID.get_manufacturerID();
  if (manufacturerId != bq2573a::MANUFACTURER_ID) {
    // wrong manufacturer id
    return false;
  }

  byte deviceId = BQ25703Areg.deviceID.get_deviceID();
  if (deviceId != bq2573a::DEVICE_ID) {
    // wrong device id
    return false;
  }

  return true;
}

bool is_usb_powered() {
  return (NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk) != 0x00;
}
bool is_powered_on() { return (digitalRead(CHARGE_OK) == HIGH); }

bool enable_charge() {
  static bool isChargeEnabled = false;
  static bool isChargeResetted = false;

  // prevent flickering when the charge is almost over
  static bool voltageHysteresisActivated = false;

  bool shouldCharge = true;
  // battery high enough;: stop charge
  if (battery::get_battery_level() >= 99) {
    shouldCharge = false;
    voltageHysteresisActivated = true;
  }
  // do not start charge back until we drop below the target threshold
  else if (voltageHysteresisActivated) {
    shouldCharge = false;
    if (battery::get_battery_level() < 97) {
      voltageHysteresisActivated = false;
    }
  }

  // temperature too high, stop charge
  if ((AlertManager.current() & Alerts::TEMP_CRITICAL) != 0x00) {
    shouldCharge = false;
  }

  // should not charge battery, or charge is not enabled
  if (!shouldCharge) {
    // stop charge already invoked ?
    if (not isChargeResetted) {
      disable_charge();
      isChargeResetted = true;
    }

    // dont run the charge functions
    return false;
  }

  // reset hysteresis
  voltageHysteresisActivated = false;

  // Setting the max voltage that the charger will charge the batteries up
  // to
  if (is_powered_on()) {
    if (not isChargeEnabled) {
      isChargeEnabled = true;

      // set the pd negociation
      PD_UFP.init(CHARGE_INT, PD_POWER_OPTION_MAX_20V);

      // Setting the max voltage that the charger will charge the batteries up
      // to
      BQ25703Areg.maxChargeVoltage.set_voltage(16750);  // max battery voltage
      BQ25703Areg.chargeCurrent.set_current(1000);  // charge current regulation

      // Set the watchdog timer to not have a timeout
      // When changing bitfield values, call the writeRegEx function
      // This is so you can change all the bits you want before sending out
      // the byte.
      BQ25703Areg.chargeOption0.set_WDTMR_ADJ(0);
      charger.writeRegEx(BQ25703Areg.chargeOption0);

      // Set the ADC on IBAT to record values
      BQ25703Areg.chargeOption1.set_EN_IBAT(1);
      charger.writeRegEx(BQ25703Areg.chargeOption1);

      // Set ADC to make continuous readings. (uses more power)
      BQ25703Areg.aDCOption.set_ADC_CONV(1);
      // Set individual ADC registers to read. All have default off.
      BQ25703Areg.aDCOption.set_EN_ADC_IDCHG(1);
      BQ25703Areg.aDCOption.set_EN_ADC_ICHG(1);
      // Once bits have been twiddled, send bytes to device
      charger.writeRegEx(BQ25703Areg.aDCOption);

      BQ25703Areg.maxChargeVoltage.set_voltage(16750);
    }

    // run pd negociation
    PD_UFP.run();

    // update the charge current
    BQ25703Areg.chargeCurrent.set_current(1000);

    // flag to signal that the charge must be stopped
    isChargeResetted = false;
  } else {
    isChargeEnabled = false;
  }

  return isChargeEnabled;
}

void disable_charge() {
  // set charge current to 0
  BQ25703Areg.chargeOption1.set_EN_IBAT(0);
  charger.writeRegEx(BQ25703Areg.chargeOption1);
  BQ25703Areg.aDCOption.set_ADC_CONV(0);
  BQ25703Areg.aDCOption.set_EN_ADC_IDCHG(0);
  BQ25703Areg.aDCOption.set_EN_ADC_ICHG(0);
  charger.writeRegEx(BQ25703Areg.aDCOption);

  BQ25703Areg.chargeCurrent.set_current(0);
}

void set_system_voltage(const float voltage) {
  const uint16_t v = voltage * 1000.0;
  BQ25703Areg.minSystemVoltage.set_voltage(v);
}

}  // namespace charger