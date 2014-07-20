#ifndef STM32_H
#define STM32_H

/**
 * @name    STM32-specific EXT channel modes
 * @{
 */
#define EXT_MODE_GPIO_MASK  0xF0        /**< @brief Port field mask.        */
#define EXT_MODE_GPIO_OFF   4           /**< @brief Port field offset.      */
#define EXT_MODE_GPIOA      0x00        /**< @brief GPIOA identifier.       */
#define EXT_MODE_GPIOB      0x10        /**< @brief GPIOB identifier.       */
#define EXT_MODE_GPIOC      0x20        /**< @brief GPIOC identifier.       */
#define EXT_MODE_GPIOD      0x30        /**< @brief GPIOD identifier.       */
#define EXT_MODE_GPIOE      0x40        /**< @brief GPIOE identifier.       */
#define EXT_MODE_GPIOF      0x50        /**< @brief GPIOF identifier.       */
#define EXT_MODE_GPIOG      0x60        /**< @brief GPIOG identifier.       */
#define EXT_MODE_GPIOH      0x70        /**< @brief GPIOH identifier.       */
#define EXT_MODE_GPIOI      0x80        /**< @brief GPIOI identifier.       */
/** @} */

/**
 * Map GPIOs to IOPORTs.
 */
#define GPIOA IOPORT1
#define GPIOB IOPORT2
#define GPIOC IOPORT3
#define GPIOD IOPORT4

/**
 * @brief   Managed RAM size.
 * @details Size of the RAM area to be managed by the OS. If set to zero
 *          then the whole available RAM is used. The core memory is made
 *          available to the heap allocator and/or can be used directly through
 *          the simplified core memory allocator.
 *
 * @note    In order to let the OS manage the whole RAM the linker script must
 *          provide the @p __heap_base__ and @p __heap_end__ symbols.
 * @note    Requires @p CH_USE_MEMCORE.
 */
#ifdef CH_MEMCORE_SIZE
# undef CH_MEMCORE_SIZE
#endif
#define CH_MEMCORE_SIZE                 0x40000


#endif /* STM32_H */
