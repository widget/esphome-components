#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace axp202 {

struct AXP202Store {
  ISRInternalGPIOPin irq;
  volatile bool trigger{false};

  static void gpio_intr(AXP202Store *arg);
};

class AXP202Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void set_interrupt_pin(InternalGPIOPin *pin) { this->interrupt_pin_ = pin; }
  void set_battery_level_sensor(sensor::Sensor *battery_level_sensor) { battery_level_sensor_ = battery_level_sensor; }
  void set_battery_voltage_sensor(sensor::Sensor *battery_voltage_sensor) {
    battery_voltage_sensor_ = battery_voltage_sensor;
  }
  void set_usb_binary_sensor(binary_sensor::BinarySensor *usb) { usb_ = usb; }
  void set_charging_binary_sensor(binary_sensor::BinarySensor *charging) { charging_ = charging; }
  void set_button_binary_sensor(binary_sensor::BinarySensor *button) { button_ = button; }
  void set_bus_voltage_sensor(sensor::Sensor *bus_voltage_sensor) { bus_voltage_sensor_ = bus_voltage_sensor; }
  void set_brightness(float brightness) { brightness_ = brightness; }

  // ========== INTERNAL METHODS ==========
  // (In most use cases you won't need these)
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void update() override;

  void SetLDO2(bool State);
  void SetLDO3(bool State);

 protected:
  sensor::Sensor *battery_level_sensor_{nullptr};
  sensor::Sensor *battery_voltage_sensor_{nullptr};
  sensor::Sensor *bus_voltage_sensor_{nullptr};
  binary_sensor::BinarySensor *button_{nullptr};
  binary_sensor::BinarySensor *charging_{nullptr};
  binary_sensor::BinarySensor *usb_{nullptr};
  float brightness_{1.0f};
  float curr_brightness_{-1.0f};

  InternalGPIOPin *interrupt_pin_{nullptr};
  AXP202Store store_{};

  /**
   * LDO2: Display backlight
   * LDO3: Audio Module
   * LDO4: NO USE
   * DCDC3: "ESP32 (can't close)""
   */
  void begin(bool disableLDO2 = false, bool disableLDO3 = false);
  void UpdateBrightness();
  bool GetBatState();
  bool GetVBusState();
  /*
  uint8_t  GetBatData();

  void  EnableCoulombcounter(void);
  void  DisableCoulombcounter(void);
  void  StopCoulombcounter(void);
  void  ClearCoulombcounter(void);
  uint32_t GetCoulombchargeData(void);
  uint32_t GetCoulombdischargeData(void);
  float GetCoulombData(void);

  uint16_t GetVbatData(void) __attribute__((deprecated));
  uint16_t GetIchargeData(void) __attribute__((deprecated));
  uint16_t GetIdischargeData(void) __attribute__((deprecated));
  uint16_t GetTempData(void) __attribute__((deprecated));
  uint32_t GetPowerbatData(void) __attribute__((deprecated));
  uint16_t GetVinData(void) __attribute__((deprecated));
  uint16_t GetIinData(void) __attribute__((deprecated));
  uint16_t GetVusbinData(void) __attribute__((deprecated));
  uint16_t GetIusbinData(void) __attribute__((deprecated));
  uint16_t GetVapsData(void) __attribute__((deprecated));
  uint8_t GetBtnPress(void);

    // -- sleep
  void SetSleep(void);
  void DeepSleep(uint64_t time_in_us = 0);
  void LightSleep(uint64_t time_in_us = 0);

  // void SetChargeVoltage( uint8_t );
  void  SetChargeCurrent( uint8_t );
  float GetBatPower();
  float GetAPSVoltage();
  float GetBatCoulombInput();
  float GetBatCoulombOut();
  uint8_t GetWarningLevel(void);
  void SetCoulombClear();
  void SetAdcState(bool State);

  void PowerOff();
  */

  float GetBatVoltage();
  float GetBatChargeCurrent();
  uint8_t GetFuelGauge();
  float GetBatCurrent();
  float GetVinVoltage();
  float GetVinCurrent();
  float GetVBusVoltage();
  float GetVBusCurrent();
  float GetTempInternal();

  void SetLDO4(bool State);

  void clearInterrupts();
  bool Write1Byte(uint8_t Addr, uint8_t Data);
  uint8_t Read8bit(uint8_t Addr);
  uint16_t Read12Bit(uint8_t Addr);
  uint16_t Read13Bit(uint8_t Addr);
  uint16_t Read16bit(uint8_t Addr);
  uint32_t Read24bit(uint8_t Addr);
  uint32_t Read32bit(uint8_t Addr);
  void ReadBuff(uint8_t Addr, uint8_t Size, uint8_t *Buff);
};

}  // namespace axp202
}  // namespace esphome
