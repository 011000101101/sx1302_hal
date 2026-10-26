#!/bin/sh
set -eu
# Build only in the caller's isolated HAL copy. No hardware access occurs.
cd "$(dirname "$0")/.."
${CC:-cc} -O1 -g -ffunction-sections -fdata-sections -Iinc -I../libtools/inc \
  tst/test_loragw_poc_registers.c src/loragw_sx1302.c -Wl,--gc-sections -lm -o test_loragw_poc_registers
./test_loragw_poc_registers
${CC:-cc} -O1 -g -Iinc -I../libtools/inc tst/test_loragw_poc_config.c \
  libloragw.a ../libtools/libparson.a ../libtools/libtinymt32.a -lm -lrt -o test_loragw_poc_config
./test_loragw_poc_config
