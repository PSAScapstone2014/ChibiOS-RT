# List of all the Posix platform files.
PLATFORMSRC = ${CHIBIOS}/os/hal/platforms/Posix/hal_lld.c \
              ${CHIBIOS}/os/hal/platforms/Posix/pal_lld.c \
              ${CHIBIOS}/os/hal/platforms/Posix/serial_lld.c \
              ${CHIBIOS}/os/hal/platforms/Posix/ext_lld.c \
              ${CHIBIOS}/os/hal/platforms/Posix/spi_lld.c \
              ${CHIBIOS}/os/hal/platforms/Posix/i2c_lld.c \
              ${CHIBIOS}/os/hal/platforms/Posix/rtc_lld.c \

# Required include directories
PLATFORMINC = ${CHIBIOS}/os/hal/platforms/Posix
