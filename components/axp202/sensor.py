
import logging

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import (
    CONF_BATTERY_LEVEL,
    CONF_BATTERY_VOLTAGE,
    CONF_BUS_VOLTAGE,
    CONF_ID,
    CONF_SPEAKER,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_VOLTAGE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_BATTERY,
    STATE_CLASS_MEASUREMENT,
    UNIT_PERCENT,
    UNIT_VOLT,
)

DEPENDENCIES = ['i2c']

LOGGER = logging.getLogger(__name__)

axp202_ns = cg.esphome_ns.namespace('axp202')

AXP202Component = axp202_ns.class_('AXP202Component', cg.PollingComponent, i2c.I2CDevice)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(AXP202Component),
    cv.Optional(CONF_BATTERY_VOLTAGE): sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_BUS_VOLTAGE): sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_BATTERY,
        state_class=STATE_CLASS_MEASUREMENT,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon=ICON_BATTERY,
    ),
    cv.Optional("backlight", default=False): cv.boolean,
    cv.Optional(CONF_SPEAKER, default=False): cv.boolean,
}).extend(cv.polling_component_schema('60s')).extend(i2c.i2c_device_schema(0x35))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    # Control power domains
    if speaker := config.get(CONF_SPEAKER):
        LOGGER.info("Setting LD03 " + repr(speaker))
        cg.add(var.SetLDO3(speaker))
    if backlight := config.get("backlight"):
        LOGGER.info("Setting LD02 " + repr(backlight))
        cg.add(var.SetLDO2(backlight))

    # Enable optional voltage reporting
    if batt_voltage_config := config.get(CONF_BATTERY_VOLTAGE):
        sens = await sensor.new_sensor(batt_voltage_config)
        cg.add(var.set_battery_voltage_sensor(sens))

    if bus_voltage_config := config.get(CONF_BUS_VOLTAGE):
        sens = await sensor.new_sensor(bus_voltage_config)
        cg.add(var.set_bus_voltage_sensor(sens))

    if battery_level_config := config.get(CONF_BATTERY_LEVEL):
        sens = await sensor.new_sensor(battery_level_config)
        cg.add(var.set_battery_level_sensor(sens))
