import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor

from . import CONF_AXP202_ID, AXP202Component

from esphome.const import (
    CONF_BUTTON,
    DEVICE_CLASS_BATTERY_CHARGING,
    DEVICE_CLASS_POWER,
    ICON_POWER,
)

TYPES = ["charging", "usb", CONF_BUTTON]

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_AXP202_ID): cv.use_id(AXP202Component),
            cv.Optional("charging"): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_BATTERY_CHARGING
            ),
            cv.Optional("usb"): binary_sensor.binary_sensor_schema(
                icon=ICON_POWER, device_class=DEVICE_CLASS_POWER
            ),
            cv.Optional(CONF_BUTTON): binary_sensor.binary_sensor_schema(),
        }
    ).extend(cv.COMPONENT_SCHEMA)
)


async def setup_conf(config, key, parent):
    if sensor_config := config.get(key):
        var = await binary_sensor.new_binary_sensor(sensor_config)
        cg.add(getattr(parent, f"set_{key}_binary_sensor")(var))


async def to_code(config):
    parent = await cg.get_variable(config[CONF_AXP202_ID])
    for key in TYPES:
        await setup_conf(config, key, parent)
