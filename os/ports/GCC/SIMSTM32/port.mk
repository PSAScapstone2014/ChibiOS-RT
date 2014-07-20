# List of the ChibiOS/RT SIMSTM32 port files.
PORTSRC = ${CHIBIOS}/os/ports/GCC/SIMSTM32/chcore.c \
          ${CHIBIOS}/os/various/memstreams.c

BOARDSRC = ${CHIBIOS}/boards/simulator/board.c

PORTASM =

PORTINC = ${CHIBIOS}/os/ports/GCC/SIMSTM32 \
          ${CHIBIOS}/boards/simulator

SZ = size
