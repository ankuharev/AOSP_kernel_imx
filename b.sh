#!/bin/bash

cd ~/android
cp -v ~/AOSP/kernel_imx/arch/arm/boot/dts/imx6qd-iwg15s-pico_ldo*.dtb .
cp -v ~/AOSP/kernel_imx/arch/arm/boot/dts/.*iwg15s*.dtb.dts.tmp .
cp -v ~/AOSP/kernel_imx/arch/arm/boot/zImage .
cp -v ~/AOSP/kernel_imx/.config .
git add .


