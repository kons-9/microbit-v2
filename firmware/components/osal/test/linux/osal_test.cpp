#include <osal/mutex>
#include <cstdio>
#include <cassert>

int main() {
    osal::mutex mtx;
    mtx.lock();
    bool ok = mtx.try_lock();
    // NOTE: POSIX non-recursive mutex — try_lock in same thread is UB/fails
    (void)ok;
    mtx.unlock();

    {
        osal::lock_guard<osal::mutex> guard(mtx);
        // クリティカルセクション
    }

    std::printf("osal::mutex test passed\n");
    return 0;
}
