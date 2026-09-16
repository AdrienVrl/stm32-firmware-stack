# stm32-firmware-stack
This is a personal project to learn embedded development using a STM32 Nucleo board.  The goal was to be more familiar with STM32 development, FreeRTOS and TFLite/STM32Cube.AI.

## What this project contains 
This project implements:
- A register level BSP (no HAL/CubeMX) with GPIO/UART/SPI/I2C/PWM/I2S drivers, clock management, DMA and timer input capture.
- A small FreeRTOS application making use of concurrent tasks, semaphores and queues to create a small sensor pipeline.
- A bootloader with a protocol to update the application via UART.
- An audio pipeline including preprocessing and a STM32Cube.AI network for speech keyword spotting.
In addition, the project includes unity unit tests, OpenOCD/GDB debugging and TensorFlow network training.

## Architecture
### Main directories
- `core/`: core functions such as SystemInit(), SystemClock_Config(), printf() syscall or intrinsics, as well as the linker script
- `bsp/`: BSP headers and source files for each driver
- `bootloader/`: bootloader's update protocol and linker script
- `app/`: main FreeRTOS application
- `ml/`: audio processing frontend, generated network and training material
- `tests/`: unit tests for drivers and the bootloader
- `tools/`: openOCD config and bootloader flash update script

### FreeRTOS tasks
- `vSensorReaderTask`: reads accelerometer/gyroscope values via I2C, priority 4
- `vSensorProcessorTask`: processes sensor values, priority 3
- `vSensorOutputTask`: prints sensor values via UART, priority 1
- `vHeartBeatTask`: blinks an LED on the board, priority 2
- `vStatsTask`: prints remaining stack size for each task, priority 1
- `vButtonTask`: triggers voice recording on button press, priority 1
- `vInferenceTask`: processes audio via frontend and network and makes keyword prediction, priority 1
- `vWatchdogTask`: watches for task misses, priority 5
`vWatchdogTask` has higher priority to ensure that it reports correct misses on time, `vSensorReaderTask`, `vSensorProcessorTask` and `vHeartBeatTask` have priorities 4, 3 and 2 respectively to ensure regular peripheral i/o. On the other hand, `vInferenceTask` has a lower priority to prevent it from blocking other tasks during the long audio buffer read. `vButtonTask`, `vSensorOutputTask` and `vStatsTask` share priority 1 since they can be delayed by a `vInferenceTask` after a button press. Indeed, delaying the button task itself should not be an issue (the button shouldn't be pressed twice without inference in between) and the other tasks are just text output.

### Model architecture: DS-CNN-S

| Layer | Config |
|---|---|
| Input | `(49, 10, 1)` — 49 MFCC frames × 10 coefficients × 1 channel |
| Conv2D | 64 filters, kernel `(10,4)`, stride `(2,2)` → BatchNorm → ReLU |
| Depthwise-separable block × 4 | DepthwiseConv2D `(3,3)`, stride `(1,1)` → BatchNorm → ReLU → Conv2D `(1,1)`, 64 filters → BatchNorm → ReLU |
| GlobalAveragePooling2D | |
| Dense | 12 outputs, softmax |

- **Parameters:** 22,604 (~23.8 KiB)
- **MACC per inference:** 2,665,536
- **Output classes (12):** `yes, no, up, down, left, right, on, off, stop, go, unknown, silence`

**Quantization (INT8, full-integer):**
- Input: `int8 (1,49,10,1)`, scale `0.6043851971626282`, zero-point `76`
- Output: `int8 (1,12)`, scale `0.00390625`, zero-point `-128`

**Training:** Google Speech Commands v2, feature extraction matching ARM's DS-CNN-S recipe (16kHz, 40ms window / 20ms hop, 10 MFCC coefficients) rather than a from-scratch
design — chosen so the on-device CMSIS-DSP feature pipeline could match a well-documented reference rather than an invented one.

## Memory map
- bootloader region:
  ``FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 32K
  SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K``
- application region:
  ``FLASH (rx)  : ORIGIN = 0x08008000, LENGTH = 480K
  SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K``

- bootloader memory usage:
  `` Memory region      Used Size  Region Size  %age Used
           FLASH:       23896 B        32 KB     72.92%
            SRAM:        2624 B       128 KB      2.00% ``
- application memory usage:
`` Memory region       Used Size  Region Size  %age Used
           FLASH:      254672 B       480 KB     51.81%
            SRAM:       85248 B       128 KB     65.04%`` 

## Build and flash instructions
Requirements:
- `arm-none-eabi-gcc` (this project builds with 14.x)
- `cmake` >= 3.20
- `openocd` (flashing and debugging over ST-Link)
- Git with submodule support: this repo vendors several dependencies (FreeRTOS-Kernel, CMSIS-DSP, CMSIS-Core, Unity/CMock/CException for host tests), so clone with:
  ```bash
  git clone --recurse-submodules <repo-url>
  # or, if you already cloned without it:
  git submodule update --init --recursive
  ```
- A native host C compiler (gcc or clang) — only needed for the host-side unit test suite, not for building firmware.

Building and flashing the firmware:
```bash
cmake -B build/arm -DTARGET_PLATFORM=arm -DCMAKE_BUILD_TYPE=Release
cmake --build build/arm --target flash_firmware
```
Host-side unit tests
```bash
cmake -B build/host -DTARGET_PLATFORM=host
cmake --build build/host
ctest --test-dir build/host
```
- DCMAKE_BUILD_TYPE=Debug: -Og -g3 -DDEBUG. Keeps ASSERT() checks active (see core/include/common.h). Use this for day-to-day development and debugging.
- DCMAKE_BUILD_TYPE=Release: -Os -DNDEBUG. Assertions compile out entirely. Use this for anything performance-sensitive, and for reproducing the timing numbers.
    
## Hardware setup
- STM32F446RET6 Nucleo Board
- MPU6050 I2C accelerometer and gyroscope
  - Wire VCC to 3V3, GND to GND, SCL to PB8 and SDA to PB9
- INMP441 I2S microphone
  - Wire VDD to 3V3, GND to GND, L/R to GND, SCK to PC10, WS to PA4 and SD to PC12

## Measurements:
- Float model test accuracy: 0.8075
- Quantized model test accuracy: 0.8044
- flash footprint: 23896 B bootloader, 254672 B app
- RAM footprint: 2624 B bootloader, 85248 B app

## Roadmap
- Improve network accuracy
- Improve latency and memory usage
- frontend audio processing: ~166.56ms
- end to end keyword spotting latency: ~216.17 ms
All done with DCMAKE_BUILD_TYPE=Release


