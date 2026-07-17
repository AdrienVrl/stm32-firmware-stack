# Debugging

To debug a program running on the board, first launch openocd:
```
openocd -f tools/openocd/nucleo_f446re.cfg
```

Then, in another terminal, start a gdb session:
```
arm-none-eabi-gdb
```
Inside gdb, connect to the openocd server:
```
target extended-remote localhost:3333
```
You can now pass openocd commands:
```
monitor reset halt
info registers
print $pc
