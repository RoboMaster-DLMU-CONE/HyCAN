#ifndef SPINLOCK_HPP
#define SPINLOCK_HPP

#include <atomic>

namespace HyCAN::Util
{
    class SpinLock
    {
    public:
        void lock() noexcept
        {
            for (;;)
            {
                if (!lock_.exchange(true, std::memory_order_acquire))
                {
                    return;
                }
                while (lock_.load(std::memory_order_relaxed))
                {
                    __builtin_ia32_pause();
                    // Currently don't support Arm machine. Need someone help.
                    // std::this_thread::yield(); performance is bad.
                }
            }
        }

        bool try_lock() noexcept
        {
            return !lock_.load(std::memory_order_relaxed) && !lock_.exchange(true, std::memory_order_acquire);
        }

        void unlock() noexcept
        {
            lock_.store(false, std::memory_order_release);
        }

    private:
        std::atomic<bool> lock_{false};
    };
}

#endif //SPINLOCK_HPP
