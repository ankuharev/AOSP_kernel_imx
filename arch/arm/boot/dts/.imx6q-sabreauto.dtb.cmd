cmd_arch/arm/boot/dts/imx6q-sabreauto.dtb := arm-eabi-gcc -E -Wp,-MD,arch/arm/boot/dts/.imx6q-sabreauto.dtb.d.pre.tmp -nostdinc -I/home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts -I/home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts/include -undef -D__DTS__ -x assembler-with-cpp -o arch/arm/boot/dts/.imx6q-sabreauto.dtb.dts.tmp arch/arm/boot/dts/imx6q-sabreauto.dts ; /home/osboxes/AOSP/kernel_imx/scripts/dtc/dtc -O dtb -o arch/arm/boot/dts/imx6q-sabreauto.dtb -b 0 -i arch/arm/boot/dts/  -d arch/arm/boot/dts/.imx6q-sabreauto.dtb.d.dtc.tmp arch/arm/boot/dts/.imx6q-sabreauto.dtb.dts.tmp ; cat arch/arm/boot/dts/.imx6q-sabreauto.dtb.d.pre.tmp arch/arm/boot/dts/.imx6q-sabreauto.dtb.d.dtc.tmp > arch/arm/boot/dts/.imx6q-sabreauto.dtb.d

source_arch/arm/boot/dts/imx6q-sabreauto.dtb := arch/arm/boot/dts/imx6q-sabreauto.dts

deps_arch/arm/boot/dts/imx6q-sabreauto.dtb := \
  arch/arm/boot/dts/imx6q.dtsi \
  arch/arm/boot/dts/imx6q-pinfunc.h \
  arch/arm/boot/dts/imx6qdl.dtsi \
  arch/arm/boot/dts/skeleton.dtsi \
  /home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts/include/dt-bindings/gpio/gpio.h \
  arch/arm/boot/dts/imx6qdl-sabreauto.dtsi \

arch/arm/boot/dts/imx6q-sabreauto.dtb: $(deps_arch/arm/boot/dts/imx6q-sabreauto.dtb)

$(deps_arch/arm/boot/dts/imx6q-sabreauto.dtb):
