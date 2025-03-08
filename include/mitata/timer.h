#pragma once

#include <atomic>
#include <chrono>
#include <thread>

#include "mitata/types.h"

namespace mitata {

namespace timeout {
static std::atomic<bool> g_timeout_requested{false};
static std::thread* g_timer_thread = nullptr;
static std::atomic<u64> g_current_timeout{0};
static std::atomic<bool> g_thread_running{false};
static std::chrono::time_point<std::chrono::high_resolution_clock> g_group_start_time;

inline void request() {
  g_timeout_requested.store(true);
}

inline bool requested() {
  return g_timeout_requested.load();
}

inline void reset() {
  g_timeout_requested.store(false);
  g_current_timeout.store(0);
}

inline void ensure_timer_thread_exists() {
  if (g_timer_thread == nullptr) {
    g_thread_running.store(true);
    g_timer_thread = new std::thread([]() {
      while (g_thread_running.load()) {
        u64 timeout_ns = g_current_timeout.load();

        if (timeout_ns > 0) {
          auto current_time = std::chrono::high_resolution_clock::now();
          auto elapsed =
            std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - g_group_start_time)
              .count();

          if (elapsed >= timeout_ns) {
            request();
          }
        }

        std::this_thread::sleep_for(std::chrono::nanoseconds(10));
      }
    });
  }
}

// Start the group timer with a new timeout
inline void start_group_timer(u64 timeout_ns) {
  if (timeout_ns == 0)
    return;

  ensure_timer_thread_exists();
  g_timeout_requested.store(false);
  g_group_start_time = std::chrono::high_resolution_clock::now();
  g_current_timeout.store(timeout_ns);
}

inline void stop_timer() {
  g_current_timeout.store(0);
  reset();
}

inline void cleanup() {
  if (g_timer_thread) {
    g_thread_running.store(false);
    g_timer_thread->join();
    delete g_timer_thread;
    g_timer_thread = nullptr;
  }
}

class ScopedTimer {
 public:
  explicit ScopedTimer(u64 timeout_ns) {
    reset();
    start_group_timer(timeout_ns);
  }

  ~ScopedTimer() {
    stop_timer();
  }

  ScopedTimer(const ScopedTimer&) = delete;
  ScopedTimer& operator=(const ScopedTimer&) = delete;

  ScopedTimer(ScopedTimer&&) = default;
  ScopedTimer& operator=(ScopedTimer&&) = default;
};

}  // namespace timeout

}  // namespace mitata