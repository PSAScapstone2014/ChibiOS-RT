/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012,2013 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

                                      ---

    A special exception to the GPL can be applied should you wish to distribute
    a combined work that includes ChibiOS/RT, without being obliged to provide
    the source code for any proprietary components. See the file exception.txt
    for full details of how and when the exception can be applied.
*/

/**
 * @addtogroup SIMSTM32_CORE
 * @{
 */

#include <stdlib.h>
#include <sys/time.h>

#include "ch.h"
#include "hal.h"

static struct timeval nextcnt;
static struct timeval tick = {0, 1000000 / CH_FREQUENCY};

void _port_init(void) {
  gettimeofday(&nextcnt, NULL);
  timeradd(&nextcnt, &tick, &nextcnt);
}

/**
 * @brief   Kernel-lock action.
 * @details Usually this function just disables interrupts but may perform more
 *          actions.
 */
void _port_lock(void) {
}

/**
 * @brief   Kernel-unlock action.
 * @details Usually this function just enables interrupts but may perform more
 *          actions.
 */
void _port_unlock(void) {
}

/**
 * @brief   Kernel-lock action from an interrupt handler.
 * @details This function is invoked before invoking I-class APIs from
 *          interrupt handlers. The implementation is architecture dependent,
 *          in its simplest form it is void.
 */
void _port_lock_from_isr(void) {
}

/**
 * @brief   Kernel-unlock action from an interrupt handler.
 * @details This function is invoked after invoking I-class APIs from interrupt
 *          handlers. The implementation is architecture dependent, in its
 *          simplest form it is void.
 */
void _port_unlock_from_isr(void) {
}

/**
 * Performs a context switch between two threads.
 * @param otp the thread to be switched out
 * @param ntp the thread to be switched in
 */
__attribute__((used))
static void __dummy(Thread *ntp, Thread *otp) {
  (void)ntp; (void)otp;

  asm volatile (
#if defined(WIN32)
                ".globl @port_switch@8                          \n\t"
                "@port_switch@8:"
#elif defined(__APPLE__)
                ".globl _port_switch                            \n\t"
                "_port_switch:"
#else
                ".globl port_switch                             \n\t"
                "port_switch:"
#endif
                "push    %ebp                                   \n\t"
                "push    %esi                                   \n\t"
                "push    %edi                                   \n\t"
                "push    %ebx                                   \n\t"
                "movl    %esp, 12(%edx)                         \n\t"
                "movl    12(%ecx), %esp                         \n\t"
                "pop     %ebx                                   \n\t"
                "pop     %edi                                   \n\t"
                "pop     %esi                                   \n\t"
                "pop     %ebp                                   \n\t"
                "ret");
}

/**
 * Halts the system. In this implementation it just exits the simulation.
 */
__attribute__((fastcall))
void port_halt(void) {

  exit(2);
}

/**
 * @brief   Start a thread by invoking its work function.
 * @details If the work function returns @p chThdExit() is automatically
 *          invoked.
 */
__attribute__((cdecl, noreturn))
void _port_thread_start(msg_t (*pf)(void *), void *p) {

  chSysUnlock();
  chThdExit(pf(p));
  while(1);
}

/**
 * @brief Interrupt simulation.
 */
void ChkIntSources(void) {
  struct timeval tv;

#if CH_DEMO
  if (sd_lld_interrupt_pending()) {
    dbg_check_lock();
    if (chSchIsPreemptionRequired())
      chSchDoReschedule();
    dbg_check_unlock();
    return;
  }
#endif

  gettimeofday(&tv, NULL);
  if (timercmp(&tv, &nextcnt, >=)) {
    timeradd(&nextcnt, &tick, &nextcnt);

    CH_IRQ_PROLOGUE();

    chSysLockFromIsr();
    chSysTimerHandlerI();
    chSysUnlockFromIsr();

    CH_IRQ_EPILOGUE();

    dbg_check_lock();
    if (chSchIsPreemptionRequired())
      chSchDoReschedule();
    dbg_check_unlock();
  }
}

/** @} */
