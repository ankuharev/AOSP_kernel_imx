cmd_arch/arm/boot/dts/imx27-phytec-phycore.dtb := arm-eabi-gcc -E -Wp,-MD,arch/arm/boot/dts/.imx27-phytec-phycore.dtb.d.pre.tmp -nostdinc -I/home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts -I/home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts/include -undef -D__DTS__ -x assembler-with-cpp -o arch/arm/boot/dts/.imx27-phytec-phycore.dtb.dts.tmp arch/arm/boot/dts/imx27-phytec-phycore.dts ; /home/osboxes/AOSP/kernel_imx/scripts/dtc/dtc -O dtb -o arch/arm/boot/dts/imx27-phytec-phycore.dtb -b 0 -i arch/arm/boot/dts/  -d arch/arm/boot/dts/.imx27-phytec-phycore.dtb.d.dtc.tmp arch/arm/boot/dts/.imx27-phytec-phycore.dtb.dts.tmp ; cat arch/arm/boot/dts/.imx27-phytec-phycore.dtb.d.pre.tmp arch/arm/boot/dts/.imx27-phytec-phycore.dtb.d.dtc.tmp > arch/arm/boot/dts/.imx27-phytec-phycore.dtb.d

source_arch/arm/boot/dts/imx27-phytec-phycore.dtb := arch/arm/boot/dts/imx27-phytec-phycore.dts

deps_arch/arm/boot/dts/imx27-phytec-phycore.dtb := \
  arch/arm/boot/dts/imx27.dtsi \
  arch/arm/boot/dts/skeleton.dtsi \

arch/arm/boot/dts/imx27-phytec-phycore.dtb: $(deps_arch/arm/boot/dts/imx27-phytec-phycore.dtb)

$(deps_arch/arm/boot/dts/imx27-phytec-phycore.dtb):
