#include "axp202.h"
#include "esphome/core/log.h"
#include "esp_sleep.h"

namespace esphome {
namespace axp202 {

static const char *TAG = "axp202.sensor";

void AXP202Component::setup() {
  ESP_LOGD(TAG, "Starting up");
  begin(false, false);

  if (this->interrupt_pin_ != nullptr) {
    ESP_LOGD(TAG, "Setting interrupt");
    this->interrupt_pin_->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
    this->interrupt_pin_->setup();
    this->store_.irq = this->interrupt_pin_->to_isr();
    this->interrupt_pin_->attach_interrupt(AXP202Store::gpio_intr, &this->store_, gpio::INTERRUPT_FALLING_EDGE);
  } else {
    ESP_LOGW(TAG, "No interrupt pin configured!");
  }
}

void AXP202Component::publishCharging() {
  if (this->charging_) {
    uint8_t chrg = Read8bit(0x1);
    this->charging_->publish_state(chrg & 0x40);
  }
}

void AXP202Component::publishUsb() {
  if (this->usb_) {
    this->usb_->publish_state(GetVBusState());
  }
}

void AXP202Component::checkInterrupts() {
  ESP_LOGV(TAG, "Checking IRQs");

  bool force_charging_update = false;
  uint8_t irq = Read8bit(0x48);  // IRQ1
  ESP_LOGD(TAG, "IRQ1: 0x%02x", irq);

  if (irq & 0b1100) {
    // USB changed
    this->publishUsb();
    force_charging_update = true;
  }

  irq = Read8bit(0x49);  // IRQ2
  ESP_LOGD(TAG, "IRQ2: 0x%02x", irq);

  if (force_charging_update || (irq & 0b1100)) {
    // Charging changed
    this->publishCharging();
  }

  irq = Read8bit(0x4a);  // IRQ3
  ESP_LOGD(TAG, "IRQ3: 0x%02x", irq);
  if (irq & 0x3) {
    this->pek_press_ = 16;

    if (this->button_) {
      this->button_->publish_state(true);
    }
  }

  clearInterrupts();
}

void AXP202Component::loop() {
  if (this->store_.trigger) {
    ESP_LOGI(TAG, "Servicing interrupt");
    checkInterrupts();
    this->store_.trigger = false;
  }

  if (this->pek_press_ > 0) {
    this->pek_press_--;

    if (!this->pek_press_ && this->button_) {
      this->button_->publish_state(false);
    }
  }
}

void AXP202Store::gpio_intr(AXP202Store *store) { store->trigger = true; }

void AXP202Component::dump_config() {
  ESP_LOGCONFIG(TAG, "AXP202:");
  LOG_I2C_DEVICE(this);
  LOG_PIN("  Interrupt Pin: ", this->interrupt_pin_);

  LOG_SENSOR("  ", "Bus Voltage:", this->bus_voltage_sensor_);
  LOG_SENSOR("  ", "Battery Voltage:", this->battery_voltage_sensor_);
  LOG_SENSOR("  ", "Battery Current:", this->battery_current_sensor_);
  LOG_SENSOR("  ", "Battery Level:", this->battery_level_sensor_);

  LOG_BINARY_SENSOR("  ", "Battery Charging:", this->charging_);
  LOG_BINARY_SENSOR("  ", "Vusb usable:", this->usb_);
  LOG_BINARY_SENSOR("  ", "PEK (button) usable:", this->button_);

  LOG_UPDATE_INTERVAL(this);
}

float AXP202Component::get_setup_priority() const { return setup_priority::DATA; }

void AXP202Component::update() {
  bool batt_present = GetBatState();
  bool bus_present = GetVBusState();

  if (this->bus_voltage_sensor_ != nullptr) {
    if (bus_present) {
      float vbus = GetVBusVoltage();
      ESP_LOGV(TAG, "Got Bus Voltage=%.2fV", vbus);

      this->bus_voltage_sensor_->publish_state(vbus);
    } else {
      ESP_LOGD(TAG, "Bus Voltage not present");
      this->bus_voltage_sensor_->publish_state(NAN);
    }
  }

  if (this->battery_voltage_sensor_ != nullptr) {
    if (batt_present) {
      float vbat = GetBatVoltage();
      ESP_LOGV(TAG, "Got Battery Voltage=%.2fV", vbat);
      this->battery_voltage_sensor_->publish_state(vbat);
    } else {
      ESP_LOGD(TAG, "Battery not present");
      this->battery_voltage_sensor_->publish_state(NAN);
    }
  }

  if (this->battery_current_sensor_ != nullptr) {
    if (batt_present && !((Read8bit(0x1) & 0x40))) {
      this->battery_current_sensor_->publish_state(GetBatDischargeCurrent());
    } else {
      this->battery_current_sensor_->publish_state(NAN);
    }
  }

  if (this->battery_level_sensor_ != nullptr) {
    if (batt_present) {
      uint8_t batterylevel = GetFuelGauge();
      if (batterylevel > 100) {
        this->battery_level_sensor_->publish_state(NAN);
      } else {
        this->battery_level_sensor_->publish_state(float(batterylevel));
      }
    } else {
      ESP_LOGD(TAG, "Battery not present");
      this->battery_level_sensor_->publish_state(NAN);
    }
  }

  // UpdateBrightness();
}

void AXP202Component::clearInterrupts() {
  ESP_LOGV(TAG, "Clearing interrupts");
  for (uint8_t irq_addr = 0x48; irq_addr < 0x4d; irq_addr++) {
    Write1Byte(irq_addr, 0xff);
  }
}

void AXP202Component::begin(bool disableLDO2, bool disableLDO3) {
  ESP_LOGI(TAG, "Setting LDO2/3 voltages");
  // Set LDO2 & LDO3(TFT_LED & TFT) 3.0V
  Write1Byte(0x28, 0xcc);
  Write1Byte(0x29, 0x80);  // Follow LDO3IN

  /* Set ADC sample rate to 25Hz
   * Default is 25Hz, 80uA output, battery temp monitoring, flip TS pin to input when sampling.
   */
  Write1Byte(0x84, 0b00110010);

  // Battery voltage, VBUS, APS voltage, TS pin ADC.
  //  Current for battery needed for fuel gauge
  SetAdcState(0b11001011);

  // Enable bat detection, CHGLED disabled (there isn't one)
  Write1Byte(0x32, 0x46);

  // Bat charge voltage to 4.2, Current 300mA (1C of 380mAh bat)
  Write1Byte(0x33, 0xc0);

  // Configure button presses, 128mS startup time, 1S for a long press, PWROK after 64mS, shutdown on 4s press
  Write1Byte(0x36, 0x02);

  // Set temperature protection to 3.22V (useful?)
  Write1Byte(0x39, 0xfc);

  // TODO rephrase this code I think
  // Depending on configuration enable LDO2, LDO3
  uint8_t buf = (Read8bit(0x12) & 0xef) | 0x4D;
  if (disableLDO3)
    buf &= ~(1 << 6);
  // Unused bit
  // Not using DCDC2
  buf &= ~(1 << 4);
  // Not using LDO4
  buf &= ~(1 << 3);
  if (disableLDO2)
    buf &= ~(1 << 2);
  // if (disableDCDC3) // ESP32!
  //  Not using EXTEN
  buf &= ~(1 << 0);
  ESP_LOGD(TAG, "Enabling power lines: 0x%x", buf);
  if (!Write1Byte(0x12, buf)) {
    ESP_LOGW(TAG, "Failed to write!");
    mark_failed();
  }

  // Coulomb counter is disabled

  // Validate VBUS voltage to 4.45V.  Session detection off, charge/discharge resistance left off
  Write1Byte(0x8b, 0x20);

  // GPIO0 is connected to AGND, others are N/C

  // TODO How do we service an interrupt to read the pins?
  Write1Byte(0x40, 0b1100);  // IRQ1 VBus presence and loss
  Write1Byte(0x41, 0b1100);  // IRQ2 Charging presence and loss
  Write1Byte(0x42, 0b0011);  // IRQ3 just the PEK short and long press
  Write1Byte(0x43, 0x0);     // IRQ4
  // IRQ5 default off

  clearInterrupts();

  // There is a backup battery for the RTC on LDO1 which is not s/w controllable
  publishCharging();
  publishUsb();
}

bool AXP202Component::Write1Byte(uint8_t Addr, uint8_t Data) { return this->write_byte(Addr, Data); }

uint8_t AXP202Component::Read8bit(uint8_t Addr) {
  uint8_t data;
  this->read_byte(Addr, &data);
  return data;
}

uint16_t AXP202Component::Read12Bit(uint8_t Addr) {
  uint16_t Data = 0;
  uint8_t buf[2];
  ReadBuff(Addr, 2, buf);
  Data = ((buf[0] << 4) + buf[1]);  //
  return Data;
}

uint16_t AXP202Component::Read13Bit(uint8_t Addr) {
  uint16_t Data = 0;
  uint8_t buf[2];
  ReadBuff(Addr, 2, buf);
  Data = ((buf[0] << 5) + buf[1]);  //
  return Data;
}

uint16_t AXP202Component::Read16bit(uint8_t Addr) {
  uint32_t ReData = 0;
  uint8_t Buff[2];
  this->read_bytes(Addr, Buff, sizeof(Buff));
  for (int i = 0; i < sizeof(Buff); i++) {
    ReData <<= 8;
    ReData |= Buff[i];
  }
  return ReData;
}

uint32_t AXP202Component::Read24bit(uint8_t Addr) {
  uint32_t ReData = 0;
  uint8_t Buff[3];
  this->read_bytes(Addr, Buff, sizeof(Buff));
  for (int i = 0; i < sizeof(Buff); i++) {
    ReData <<= 8;
    ReData |= Buff[i];
  }
  return ReData;
}

uint32_t AXP202Component::Read32bit(uint8_t Addr) {
  uint32_t ReData = 0;
  uint8_t Buff[4];
  this->read_bytes(Addr, Buff, sizeof(Buff));
  for (int i = 0; i < sizeof(Buff); i++) {
    ReData <<= 8;
    ReData |= Buff[i];
  }
  return ReData;
}

void AXP202Component::ReadBuff(uint8_t Addr, uint8_t Size, uint8_t *Buff) { this->read_bytes(Addr, Buff, Size); }

void AXP202Component::UpdateBrightness() {
  ESP_LOGV(TAG, "Brightness=%f (Curr: %f)", brightness_, curr_brightness_);
  if (brightness_ == curr_brightness_) {
    return;
  }
  curr_brightness_ = brightness_;

  const uint8_t c_min = 7;
  const uint8_t c_max = 12;
  auto ubri = c_min + static_cast<uint8_t>(brightness_ * (c_max - c_min));

  if (ubri > c_max) {
    ubri = c_max;
  }
  uint8_t buf = Read8bit(0x28);
  ESP_LOGV(TAG, "Setting brightness to %d", ubri);
  Write1Byte(0x28, ((buf & 0x0f) | (ubri << 4)));
}

bool AXP202Component::GetBatState() {
  if (Read8bit(0x01) | 0x20)
    return true;
  else
    return false;
}

uint8_t AXP202Component::GetFuelGauge() {
  uint8_t fuel = Read8bit(0xb9);
  ESP_LOGD(TAG, "Got Battery Level=%d", fuel);
  if (fuel & 0x80) {
    return 0;
  }
  return fuel & 0x7f;
}

float AXP202Component::GetBatVoltage() {
  float ADCLSB = 1.1 / 1000.0;
  uint16_t ReData = Read12Bit(0x78);
  return ReData * ADCLSB;
}

float AXP202Component::GetBatDischargeCurrent() {
  float ADCLSB = 0.5;
  uint16_t ReData = Read13Bit(0x7C);
  return ReData * ADCLSB;
}

bool AXP202Component::GetVBusState() {
  if (Read8bit(0x0) & 0x20)
    return true;
  else
    return false;
}

float AXP202Component::GetVBusVoltage() {
  float ADCLSB = 1.7 / 1000.0;
  uint16_t ReData = Read12Bit(0x5A);
  return ReData * ADCLSB;
}

float AXP202Component::GetTempInternal() {
  float ADCLSB = 0.1;
  const float OFFSET_DEG_C = -144.7;
  uint16_t ReData = Read12Bit(0x5E);
  return OFFSET_DEG_C + ReData * ADCLSB;
}

void AXP202Component::SetLDO2(bool State) {
  uint8_t buf = Read8bit(0x12);
  if (State == true) {
    ESP_LOGV(TAG, "Enabling LDO2");
    buf = (1 << 2) | buf;
  } else {
    ESP_LOGV(TAG, "Disabling LDO2");
    buf = ~(1 << 2) & buf;
  }
  Write1Byte(0x12, buf);
}

void AXP202Component::SetLDO3(bool State) {
  uint8_t buf = Read8bit(0x12);
  if (State == true) {
    ESP_LOGV(TAG, "Enabling LDO3");
    buf = (1 << 6) | buf;
  } else {
    ESP_LOGV(TAG, "Disabling LDO3");
    buf = ~(1 << 6) & buf;
  }
  Write1Byte(0x12, buf);
}

void AXP202Component::SetLDO4(bool State) {
  uint8_t buf = Read8bit(0x12);
  if (State == true) {
    buf = (1 << 3) | buf;
  } else {
    buf = ~(1 << 3) & buf;
  }
  Write1Byte(0x12, buf);
}

void AXP202Component::SetChargeCurrent(uint8_t current) {
  uint8_t buf = Read8bit(0x33);
  buf = (buf & 0xf0) | (current & 0x07);
  Write1Byte(0x33, buf);
}

void AXP202Component::SetAdcState(uint8_t Data) { Write1Byte(0x82, Data); }

/*
uint16_t AXP202Component::GetVbatData(void){

    uint16_t vbat = 0;
    uint8_t buf[2];
    ReadBuff(0x78,2,buf);
    vbat = ((buf[0] << 4) + buf[1]); // V
    return vbat;
}

uint16_t AXP202Component::GetVinData(void)
{
    uint16_t vin = 0;
    uint8_t buf[2];
    ReadBuff(0x56,2,buf);
    vin = ((buf[0] << 4) + buf[1]); // V
    return vin;
}

uint16_t AXP202Component::GetIinData(void)
{
    uint16_t iin = 0;
    uint8_t buf[2];
    ReadBuff(0x58,2,buf);
    iin = ((buf[0] << 4) + buf[1]);
    return iin;
}

uint16_t AXP202Component::GetVusbinData(void)
{
    uint16_t vin = 0;
    uint8_t buf[2];
    ReadBuff(0x5a,2,buf);
    vin = ((buf[0] << 4) + buf[1]); // V
    return vin;
}

uint16_t AXP202Component::GetIusbinData(void)
{
    uint16_t iin = 0;
    uint8_t buf[2];
    ReadBuff(0x5C,2,buf);
    iin = ((buf[0] << 4) + buf[1]);
    return iin;
}

uint16_t AXP202Component::GetIchargeData(void)
{
    uint16_t icharge = 0;
    uint8_t buf[2];
    ReadBuff(0x7A,2,buf);
    icharge = ( buf[0] << 5 ) + buf[1] ;
    return icharge;
}

uint16_t AXP202Component::GetIdischargeData(void)
{
    uint16_t idischarge = 0;
    uint8_t buf[2];
    ReadBuff(0x7C,2,buf);
    idischarge = ( buf[0] << 5 ) + buf[1] ;
    return idischarge;
}

uint16_t AXP202Component::GetTempData(void)
{
    uint16_t temp = 0;
    uint8_t buf[2];
    ReadBuff(0x5e,2,buf);
    temp = ((buf[0] << 4) + buf[1]);
    return temp;
}

uint32_t AXP202Component::GetPowerbatData(void)
{
    uint32_t power = 0;
    uint8_t buf[3];
    ReadBuff(0x70,2,buf);
    power = (buf[0] << 16) + (buf[1] << 8) + buf[2];
    return power;
}

uint16_t AXP202Component::GetVapsData(void)
{
    uint16_t vaps = 0;
    uint8_t buf[2];
    ReadBuff(0x7e,2,buf);
    vaps = ((buf[0] << 4) + buf[1]);
    return vaps;
}

void AXP202Component::SetSleep(void)
{
    Write1Byte(0x31 , Read8bit(0x31) | ( 1 << 3)); // Power off voltag 3.0v
    Write1Byte(0x90 , Read8bit(0x90) | 0x07); // GPIO1 floating
    Write1Byte(0x82, 0x00); // Disable ADCs
    Write1Byte(0x12, Read8bit(0x12) & 0xA1); // Disable all outputs but DCDC1
}

// -- sleep
void AXP202Component::DeepSleep(uint64_t time_in_us)
{
    SetSleep();
    esp_sleep_enable_ext0_wakeup((gpio_num_t)37, 0); //LOW
    if (time_in_us > 0)
    {
        esp_sleep_enable_timer_wakeup(time_in_us);
    }
    else
    {
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    }
    (time_in_us == 0) ? esp_deep_sleep_start() : esp_deep_sleep(time_in_us);
}

void AXP202Component::LightSleep(uint64_t time_in_us)
{
    if (time_in_us > 0)
    {
        esp_sleep_enable_timer_wakeup(time_in_us);
    }
    else
    {
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    }
    esp_light_sleep_start();
}

// 0 not press, 0x01 long press, 0x02 press
uint8_t AXP202Component::GetBtnPress()
{
    uint8_t state = Read8bit(0x46);
    if(state)
    {
        Write1Byte( 0x46 , 0x03 );
    }
    return state;
}

uint8_t AXP202Component::GetWarningLevel(void)
{
    return Read8bit(0x47) & 0x01;
}


float AXP202Component::GetBatCurrent()
{
    float ADCLSB = 0.5;
    uint16_t CurrentIn = Read13Bit( 0x7A );
    uint16_t CurrentOut = Read13Bit( 0x7C );
    return ( CurrentIn - CurrentOut ) * ADCLSB;
}

float AXP202Component::GetVinVoltage() // ACIN
{
    float ADCLSB = 1.7 / 1000.0;
    uint16_t ReData = Read12Bit( 0x56 );
    return ReData * ADCLSB;
}

float AXP202Component::GetVinCurrent() // ACIN
{
    float ADCLSB = 0.625;
    uint16_t ReData = Read12Bit( 0x58 );
    return ReData * ADCLSB;
}


float AXP202Component::GetVBusCurrent()
{
    float ADCLSB = 0.375;
    uint16_t ReData = Read12Bit( 0x5C );
    return ReData * ADCLSB;
}


float AXP202Component::GetBatPower()
{
    float VoltageLSB = 1.1;
    float CurrentLCS = 0.5;
    uint32_t ReData = Read24bit( 0x70 );
    return  VoltageLSB * CurrentLCS * ReData/ 1000.0;
}


float AXP202Component::GetAPSVoltage()
{
    float ADCLSB = 1.4  / 1000.0;
    uint16_t ReData = Read12Bit( 0x7E );
    return ReData * ADCLSB;
}

*/
}  // namespace axp202
}  // namespace esphome