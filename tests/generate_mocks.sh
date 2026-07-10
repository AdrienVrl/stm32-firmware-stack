#!/bin/bash
# tests/generate_mocks.sh

CMOCK=tests/cmock/lib/cmock.rb
CONFIG=tests/cmock.yml

headers=(
    bsp/include/uart.h
    bsp/include/gpio.h
    bsp/include/spi.h
    bsp/include/i2c.h
    bsp/include/timer.h
)

for header in "${headers[@]}"; do
    echo "Generating mock for $header"
    ruby $CMOCK --config=$CONFIG $header
done
