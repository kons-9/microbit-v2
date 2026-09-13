#pragma once

/**
 * @file flash_layout.h
 * @brief Flash FS address configuration selected by target architecture.
 */

#if defined(SYSCONFIG_ARCH_MICROBIT)
#include "sysconfig/arch/microbit/flash_layout.h"
#elif defined(SYSCONFIG_ARCH_LINUX)
#include "sysconfig/arch/linux/flash_layout.h"
#else
#error "A supported sysconfig architecture must be selected"
#endif
