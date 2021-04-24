#!/bin/sh

#cp ./pico-debug/pico-debug-gimmecache.uf2 /run/media/simon/RPI-RP2
#sleep 3s

openocd -f interface/cmsis-dap.cfg -c "transport select swd" -c "adapter speed 4000" -f target/rp2040-core0.cfg &

arm-none-eabi-gdb ./build/vanny_hub.elf

