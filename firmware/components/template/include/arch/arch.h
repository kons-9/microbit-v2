#pragma once
// defineや、static inlineなど、隠蔽できないアーキテクチャに依存するコードをここに書く

#if defined(TEMPLATE_ARCH_LINUX)
#include "arch/linux/template_linux.h"
#elif defined(TEMPLATE_ARCH_MICROBIT)
#include "arch/microbit/template_microbit.h"
#else

