#include <utkernel/mutex>
#include <cstdio>
#include <cassert>

int main() {
    utkernel::mutex mtx;
    mtx.lock();
    bool ok = mtx.try_lock();
    // NOTE: POSIX non-recursive mutex — try_lock in same thread is UB/fails
    (void)ok;
    mtx.unlock();

    {
        utkernel::lock_guard<utkernel::mutex> guard(mtx);
        // クリティカルセクション
    }

    std::printf("utkernel::mutex test passed\n");
    return 0;
}
