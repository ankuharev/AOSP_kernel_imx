cmd_arch/arm/boot/dts/imx53-smd.dtb := arm-eabi-gcc -E -Wp,-MD,arch/arm/boot/dts/.imx53-smd.dtb.d.pre.tmp -nostdinc -I/home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts -I/home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts/include -undef -D__DTS__ -x assembler-with-cpp -o arch/arm/boot/dts/.imx53-smd.dtb.dts.tmp arch/arm/boot/dts/imx53-smd.dts ; /home/osboxes/AOSP/kernel_imx/scripts/dtc/dtc -O dtb -o arch/arm/boot/dts/imx53-smd.dtb -b 0 -i arch/arm/boot/dts/  -d arch/arm/boot/dts/.imx53-smd.dtb.d.dtc.tmp arch/arm/boot/dts/.imx53-smd.dtb.dts.tmp ; cat arch/arm/boot/dts/.imx53-smd.dtb.d.pre.tmp arch/arm/boot/dts/.imx53-smd.dtb.d.dtc.tmp > arch/arm/boot/dts/.imx53-smd.dtb.d

source_arch/arm/boot/dts/imx53-smd.dtb := arch/arm/boot/dts/imx53-smd.dts

deps_arch/arm/boot/dts/imx53-smd.dtb := \
  arch/arm/boot/dts/imx53.dtsi \
  arch/arm/boot/dts/skeleton.dtsi \
  arch/arm/boot/dts/imx53-pinfunc.h \

arch/arm/boot/dts/imx53-smd.dtb: $(deps_arch/arm/boot/dts/imx53-smd.dtb)

$(deps_arch/arm/boot/dts/imx53-smd.dtb):
