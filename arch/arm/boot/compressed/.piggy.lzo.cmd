cmd_arch/arm/boot/compressed/piggy.lzo := (cat arch/arm/boot/compressed/../Image | lzop -9 && printf \\100\\116\\365\\000) > arch/arm/boot/compressed/piggy.lzo || (rm -f arch/arm/boot/compressed/piggy.lzo ; false)
