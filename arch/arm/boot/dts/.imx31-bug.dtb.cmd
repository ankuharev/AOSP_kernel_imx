cmd_arch/arm/boot/dts/imx31-bug.dtb := arm-eabi-gcc -E -Wp,-MD,arch/arm/boot/dts/.imx31-bug.dtb.d.pre.tmp -nostdinc -I/home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts -I/home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts/include -undef -D__DTS__ -x assembler-with-cpp -o arch/arm/boot/dts/.imx31-bug.dtb.dts.tmp arch/arm/boot/dts/imx31-bug.dts ; /home/osboxes/AOSP/kernel_imx/scripts/dtc/dtc -O dtb -o arch/arm/boot/dts/imx31-bug.dtb -b 0 -i arch/arm/boot/dts/  -d arch/arm/boot/dts/.imx31-bug.dtb.d.dtc.tmp arch/arm/boot/dts/.imx31-bug.dtb.dts.tmp ; cat arch/arm/boot/dts/.imx31-bug.dtb.d.pre.tmp arch/arm/boot/dts/.imx31-bug.dtb.d.dtc.tmp > arch/arm/boot/dts/.imx31-bug.dtb.d

source_arch/arm/boot/dts/imx31-bug.dtb := arch/arm/boot/dts/imx31-bug.dts

deps_arch/arm/boot/dts/imx31-bug.dtb := \
  arch/arm/boot/dts/imx31.dtsi \
  arch/arm/boot/dts/skeleton.dtsi \

arch/arm/boot/dts/imx31-bug.dtb: $(deps_arch/arm/boot/dts/imx31-bug.dtb)

$(deps_arch/arm/boot/dts/imx31-bug.dtb):
