cmd_firmware/imx/epdc_E97_V110.fw.gen.o := arm-eabi-gcc -Wp,-MD,firmware/imx/.epdc_E97_V110.fw.gen.o.d  -nostdinc -isystem /home/osboxes/AOSP/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/../lib/gcc/arm-eabi/4.6.x-google/include -I/home/osboxes/AOSP/kernel_imx/arch/arm/include -Iarch/arm/include/generated  -Iinclude -I/home/osboxes/AOSP/kernel_imx/arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I/home/osboxes/AOSP/kernel_imx/include/uapi -Iinclude/generated/uapi -include /home/osboxes/AOSP/kernel_imx/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian  -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a  -include asm/unified.h -msoft-float         -c -o firmware/imx/epdc_E97_V110.fw.gen.o firmware/imx/epdc_E97_V110.fw.gen.S

source_firmware/imx/epdc_E97_V110.fw.gen.o := firmware/imx/epdc_E97_V110.fw.gen.S

deps_firmware/imx/epdc_E97_V110.fw.gen.o := \
  /home/osboxes/AOSP/kernel_imx/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
    $(wildcard include/config/thumb2/kernel.h) \

firmware/imx/epdc_E97_V110.fw.gen.o: $(deps_firmware/imx/epdc_E97_V110.fw.gen.o)

$(deps_firmware/imx/epdc_E97_V110.fw.gen.o):
