# List of all the Posix platform files.
PLATFORMSRC = ${CHIBIOS}/os/hal/platforms/Posix/hal_lld.c         \
              ${CHIBIOS}/os/hal/platforms/Posix/pal_lld.c         \
              ${CHIBIOS}/os/hal/platforms/Posix/ext_lld.c         \
              ${CHIBIOS}/os/hal/platforms/Posix/spi_lld.c         \
              ${CHIBIOS}/os/hal/platforms/Posix/i2c_lld.c         \
              ${CHIBIOS}/os/hal/platforms/Posix/rtc_lld.c         \
              ${CHIBIOS}/os/hal/platforms/Posix/gpt_lld.c         \
              ${CHIBIOS}/os/hal/platforms/Posix/pwm_lld.c         \
              ${CHIBIOS}/os/hal/platforms/Posix/sdc_lld.c         \
              ${CHIBIOS}/os/hal/platforms/Posix/adc_lld.c         \
              ${CHIBIOS}/os/hal/platforms/Posix/serial_lld.c      \
              ${CHIBIOS}/os/hal/platforms/Posix/serial_lld_demo.c

ifndef CH_DEMO
PLATFORMSRC+= ${CHIBIOS}/os/hal/platforms/Posix/simio.c
PLATFORMSRC+= ${CHIBIOS}/os/hal/platforms/Posix/simutil.c
PLATFORMSRC+= ${CHIBIOS}/os/hal/platforms/Posix/sim_preempt.c
endif

# Required include directories
PLATFORMINC = ${CHIBIOS}/os/hal/platforms/Posix
