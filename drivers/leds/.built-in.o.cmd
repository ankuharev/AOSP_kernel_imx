cmd_drivers/leds/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/leds/built-in.o drivers/leds/led-core.o drivers/leds/led-class.o drivers/leds/led-triggers.o drivers/leds/leds-gpio.o drivers/leds/trigger/built-in.o 
