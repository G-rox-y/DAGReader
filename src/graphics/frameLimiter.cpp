#include "pch.hpp"

class FrameLimiter {
public:
    explicit FrameLimiter(double target_fps)
        : target_dt_(std::chrono::nanoseconds(static_cast<int64_t>(1e9 / target_fps)))
    {
#ifdef _WIN32
    timeBeginPeriod(1);
#endif
    }

#ifdef _WIN32
    ~FrameLimiter() { timeEndPeriod(1); }
#else
    ~FrameLimiter() = default;
#endif

    // Delete copy/move
    FrameLimiter(const FrameLimiter&) = delete;
    FrameLimiter& operator=(const FrameLimiter&) = delete;

    void wait() {
        const auto now = std::chrono::steady_clock::now();
        const auto deadline = last_time_ + target_dt_;
        const auto remaining = deadline - now;

        if (remaining > std::chrono::milliseconds(4)) {
            std::this_thread::sleep_for(remaining - std::chrono::milliseconds(2));
        }

        while (std::chrono::steady_clock::now() < deadline) {
            std::this_thread::yield();
        }

        last_time_ += target_dt_;
    }

    void changeFPS(double target_fps){
        target_dt_ = std::chrono::nanoseconds(static_cast<int64_t>(1e9 / target_fps));
    }

private:
    std::chrono::nanoseconds target_dt_;
    std::chrono::steady_clock::time_point last_time_ = std::chrono::steady_clock::now();
};