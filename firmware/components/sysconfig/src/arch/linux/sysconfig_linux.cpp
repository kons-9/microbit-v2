#include <sysconfig.h>

#include <cstdlib>

namespace sysconfig {

[[noreturn]] void reboot(BootMode /*mode*/) {
    std::abort();
}

}  // namespace sysconfig
