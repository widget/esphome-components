import logging

from esphome import pins
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c

from esphome.const import (
    CONF_INTERRUPT_PIN,
    CONF_ID,
    CONF_SPEAKER,
)

LOGGER = logging.getLogger(__name__)

MULTI_CONF = True

DEPENDENCIES = ["i2c"]

axp202_ns = cg.esphome_ns.namespace("axp202")

AXP202Component = axp202_ns.class_(
    "AXP202Component", cg.PollingComponent, i2c.I2CDevice
)

CONF_AXP202_ID = "axp202_id"
CONF_BACKLIGHT = "backlight"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(AXP202Component),
            cv.Optional(CONF_BACKLIGHT, default=False): cv.boolean,
            cv.Optional(CONF_SPEAKER, default=False): cv.boolean,
            cv.Optional(CONF_INTERRUPT_PIN): cv.All(
                pins.internal_gpio_input_pin_schema
            ),
        }
    )
    .extend(i2c.i2c_device_schema(0x35))
    .extend(cv.polling_component_schema("60s"))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    # Hardcode power domains used in Lilygo T-Watch at configuration time
    # Voltage level is not controlled in YAML at all
    if speaker := config.get(CONF_SPEAKER):
        LOGGER.info("Fixing LD03 (speaker) %s", repr("on" if speaker else "off"))
        cg.add(var.SetLDO3(speaker))
    if backlight := config.get("backlight"):
        LOGGER.info("Fixing LD02 (backlight) %s", repr("on" if backlight else "off"))
        cg.add(var.SetLDO2(backlight))

    if interrupt_pin_config := config.get(CONF_INTERRUPT_PIN):
        interrupt_pin = await cg.gpio_pin_expression(interrupt_pin_config)
        cg.add(var.set_interrupt_pin(interrupt_pin))
