cmd_drivers/tty/serial/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/tty/serial/built-in.o drivers/tty/serial/serial_core.o drivers/tty/serial/imx.o drivers/tty/serial/fsl_lpuart.o 
