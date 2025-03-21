#ifndef __AXP202_H__
#define __AXP202_H__

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace axp202 {

class AXP202Component : public PollingComponent, public i2c::I2CDevice {
 public:
  // void set_batterylevel_sensor(sensor::Sensor *batterylevel_sensor) { batterylevel_sensor_ = batterylevel_sensor; }
  void set_brightness(float brightness) { brightness_ = brightness; }

  // ========== INTERNAL METHODS ==========
  // (In most use cases you won't need these)
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void update() override;

 protected:
  // sensor::Sensor *batterylevel_sensor_;
  float brightness_{1.0f};
  float curr_brightness_{-1.0f};

  /**
   * LDO2: Display backlight
   * LDO3: Audio Module
   * LDO4: NO USE
   * DCDC3: "ESP32 (can't close)""
   */
  void begin(bool disableLDO2 = false, bool disableLDO3 = false, bool disableRTC = false, bool disableDCDC1 = false,
             bool disableDCDC3 = false);
  void UpdateBrightness();
  bool GetBatState();
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
  float GetBatVoltage();
  float GetBatCurrent();
  float GetVinVoltage();
  float GetVinCurrent();
  float GetVBusVoltage();
  float GetVBusCurrent();
  float GetTempInAXP192();
  float GetBatPower();
  float GetBatChargeCurrent();
  float GetAPSVoltage();
  float GetBatCoulombInput();
  float GetBatCoulombOut();
  uint8_t GetWarningLevel(void);
  void SetCoulombClear();
  void SetAdcState(bool State);

  void PowerOff();
  */
  void SetLDO2(bool State);
  void SetLDO3(bool State);
  void SetLDO4(bool State);

  void Write1Byte(uint8_t Addr, uint8_t Data);
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

#endif