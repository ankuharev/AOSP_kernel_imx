cmd_arch/arm/lib/changebit.o := arm-eabi-gcc -Wp,-MD,arch/arm/lib/.changebit.o.d  -nostdinc -isystem /home/osboxes/AOSP/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/../lib/gcc/arm-eabi/4.6.x-google/include -I/home/osboxes/AOSP/kernel_imx/arch/arm/include -Iarch/arm/include/generated  -Iinclude -I/home/osboxes/AOSP/kernel_imx/arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I/home/osboxes/AOSP/kernel_imx/include/uapi -Iinclude/generated/uapi -include /home/osboxes/AOSP/kernel_imx/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian  -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a  -include asm/unified.h -msoft-float         -c -o arch/arm/lib/changebit.o arch/arm/lib/changebit.S

source_arch/arm/lib/changebit.o := arch/arm/lib/changebit.S

deps_arch/arm/lib/changebit.o := \
  /home/osboxes/AOSP/kernel_imx/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
    $(wildcard include/config/thumb2/kernel.h) \
  include/linux/linkage.h \
  include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
    $(wildcard include/config/kprobes.h) \
  include/linux/stringify.h \
  include/linux/export.h \
    $(wildcard include/config/have/underscore/symbol/prefix.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \
  /home/osboxes/AOSP/kernel_imx/arch/arm/include/asm/linkage.h \
  /home/osboxes/AOSP/kernel_imx/arch/arm/include/asm/assembler.h \
    $(wildcard include/config/cpu/feroceon.h) \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/cpu/use/domains.h) \
  /home/osboxes/AOSP/kernel_imx/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/arm/thumb.h) \
  /home/osboxes/AOSP/kernel_imx/arch/arm/include/uapi/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
  /home/osboxes/AOSP/kernel_imx/arch/arm/include/asm/hwcap.h \
  /home/osboxes/AOSP/kernel_imx/arch/arm/include/uapi/asm/hwcap.h \
  /home/osboxes/AOSP/kernel_imx/arch/arm/include/asm/domain.h \
    $(wildcard include/config/io/36.h) \
  /home/osboxes/AOSP/kernel_imx/arch/arm/include/asm/opcodes-virt.h \
  /home/osboxes/AOSP/kernel_imx/arch/arm/include/asm/opcodes.h \
    $(wildcard include/config/cpu/endian/be32.h) \
  arch/arm/lib/bitops.h \
  /home/osboxes/AOSP/kernel_imx/arch/arm/include/asm/unwind.h \
    $(wildcard include/config/arm/unwind.h) \

arch/arm/lib/changebit.o: $(deps_arch/arm/lib/changebit.o)

$(deps_arch/arm/lib/changebit.o):
