#ifndef SPINLOCK_HPP
#define SPINLOCK_HPP

#include <atomic>

namespace HyCAN::Util {
class SpinLock {
  public:
    void lock() noexcept {
        for (;;) {
            if (!lock_.exchange(true, std::memory_order_acquire)) {
                return;
            }
            while (lock_.load(std::memory_order_relaxed)) {
#if defined(__x86_64__) || defined(__i386__)
                __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(__arm__)
                __asm__ __volatile__("yield" ::: "memory");
#endif
            }
        }
    }

    bool try_lock() noexcept {
        return !lock_.load(std::memory_order_relaxed) &&
               !lock_.exchange(true, std::memory_order_acquire);
    }

    void unlock() noexcept { lock_.store(false, std::memory_order_release); }

  private:
    std::atomic<bool> lock_{false};
};
} // namespace HyCAN::Util

#endif // SPINLOCK_HPP
