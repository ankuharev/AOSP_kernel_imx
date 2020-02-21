cmd_arch/arm/boot/dts/imx6sl-evk.dtb := arm-eabi-gcc -E -Wp,-MD,arch/arm/boot/dts/.imx6sl-evk.dtb.d.pre.tmp -nostdinc -I/home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts -I/home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts/include -undef -D__DTS__ -x assembler-with-cpp -o arch/arm/boot/dts/.imx6sl-evk.dtb.dts.tmp arch/arm/boot/dts/imx6sl-evk.dts ; /home/osboxes/AOSP/kernel_imx/scripts/dtc/dtc -O dtb -o arch/arm/boot/dts/imx6sl-evk.dtb -b 0 -i arch/arm/boot/dts/  -d arch/arm/boot/dts/.imx6sl-evk.dtb.d.dtc.tmp arch/arm/boot/dts/.imx6sl-evk.dtb.dts.tmp ; cat arch/arm/boot/dts/.imx6sl-evk.dtb.d.pre.tmp arch/arm/boot/dts/.imx6sl-evk.dtb.d.dtc.tmp > arch/arm/boot/dts/.imx6sl-evk.dtb.d

source_arch/arm/boot/dts/imx6sl-evk.dtb := arch/arm/boot/dts/imx6sl-evk.dts

deps_arch/arm/boot/dts/imx6sl-evk.dtb := \
  arch/arm/boot/dts/imx6sl.dtsi \
  arch/arm/boot/dts/skeleton.dtsi \
  arch/arm/boot/dts/imx6sl-pinfunc.h \
  /home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts/include/dt-bindings/clock/imx6sl-clock.h \
  arch/arm/boot/dts/imx6sl-evk-common.dtsi \

arch/arm/boot/dts/imx6sl-evk.dtb: $(deps_arch/arm/boot/dts/imx6sl-evk.dtb)

$(deps_arch/arm/boot/dts/imx6sl-evk.dtb):
