cmd_arch/arm/boot/dts/imx6sx-sdb-ldo.dtb := arm-eabi-gcc -E -Wp,-MD,arch/arm/boot/dts/.imx6sx-sdb-ldo.dtb.d.pre.tmp -nostdinc -I/home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts -I/home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts/include -undef -D__DTS__ -x assembler-with-cpp -o arch/arm/boot/dts/.imx6sx-sdb-ldo.dtb.dts.tmp arch/arm/boot/dts/imx6sx-sdb-ldo.dts ; /home/osboxes/AOSP/kernel_imx/scripts/dtc/dtc -O dtb -o arch/arm/boot/dts/imx6sx-sdb-ldo.dtb -b 0 -i arch/arm/boot/dts/  -d arch/arm/boot/dts/.imx6sx-sdb-ldo.dtb.d.dtc.tmp arch/arm/boot/dts/.imx6sx-sdb-ldo.dtb.dts.tmp ; cat arch/arm/boot/dts/.imx6sx-sdb-ldo.dtb.d.pre.tmp arch/arm/boot/dts/.imx6sx-sdb-ldo.dtb.d.dtc.tmp > arch/arm/boot/dts/.imx6sx-sdb-ldo.dtb.d

source_arch/arm/boot/dts/imx6sx-sdb-ldo.dtb := arch/arm/boot/dts/imx6sx-sdb-ldo.dts

deps_arch/arm/boot/dts/imx6sx-sdb-ldo.dtb := \
  arch/arm/boot/dts/imx6sx-sdb.dts \
  /home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts/include/dt-bindings/gpio/gpio.h \
  arch/arm/boot/dts/imx6sx.dtsi \
  /home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts/include/dt-bindings/clock/imx6sx-clock.h \
  arch/arm/boot/dts/imx6sx-pinfunc.h \
  arch/arm/boot/dts/skeleton.dtsi \

arch/arm/boot/dts/imx6sx-sdb-ldo.dtb: $(deps_arch/arm/boot/dts/imx6sx-sdb-ldo.dtb)

$(deps_arch/arm/boot/dts/imx6sx-sdb-ldo.dtb):
