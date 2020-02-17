cmd_arch/arm/boot/dts/imx6dls-iwg15s-pico_ldobypass.dtb := /home/osboxes/AOSP/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-gcc -E -Wp,-MD,arch/arm/boot/dts/.imx6dls-iwg15s-pico_ldobypass.dtb.d.pre.tmp -nostdinc -I/home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts -I/home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts/include -undef -D__DTS__ -x assembler-with-cpp -o arch/arm/boot/dts/.imx6dls-iwg15s-pico_ldobypass.dtb.dts.tmp arch/arm/boot/dts/imx6dls-iwg15s-pico_ldobypass.dts ; /home/osboxes/AOSP/kernel_imx/scripts/dtc/dtc -O dtb -o arch/arm/boot/dts/imx6dls-iwg15s-pico_ldobypass.dtb -b 0 -i arch/arm/boot/dts/  -d arch/arm/boot/dts/.imx6dls-iwg15s-pico_ldobypass.dtb.d.dtc.tmp arch/arm/boot/dts/.imx6dls-iwg15s-pico_ldobypass.dtb.dts.tmp ; cat arch/arm/boot/dts/.imx6dls-iwg15s-pico_ldobypass.dtb.d.pre.tmp arch/arm/boot/dts/.imx6dls-iwg15s-pico_ldobypass.dtb.d.dtc.tmp > arch/arm/boot/dts/.imx6dls-iwg15s-pico_ldobypass.dtb.d

source_arch/arm/boot/dts/imx6dls-iwg15s-pico_ldobypass.dtb := arch/arm/boot/dts/imx6dls-iwg15s-pico_ldobypass.dts

deps_arch/arm/boot/dts/imx6dls-iwg15s-pico_ldobypass.dtb := \
  arch/arm/boot/dts/imx6dl_iwg15.dtsi \
  arch/arm/boot/dts/imx6dl-pinfunc.h \
  arch/arm/boot/dts/imx6qdl_iwg15.dtsi \
  arch/arm/boot/dts/skeleton.dtsi \
  /home/osboxes/AOSP/kernel_imx/arch/arm/boot/dts/include/dt-bindings/gpio/gpio.h \
  arch/arm/boot/dts/imx6qdl-iwg15s-pico.dtsi \

arch/arm/boot/dts/imx6dls-iwg15s-pico_ldobypass.dtb: $(deps_arch/arm/boot/dts/imx6dls-iwg15s-pico_ldobypass.dtb)

$(deps_arch/arm/boot/dts/imx6dls-iwg15s-pico_ldobypass.dtb):
