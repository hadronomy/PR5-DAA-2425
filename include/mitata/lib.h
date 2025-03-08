#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <functional>
#include <future>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>

#include "mitata/timer.h"
#include "mitata/types.h"

namespace mitata {
namespace lib {
struct k_stats {
  u64 ticks;
  f64 min, max, avg;
  std::vector<f64> samples;
  f64 p25, p50, p75, p99, p999;
  bool timeout = false;  // Add timeout flag
};

static const auto k_min_samples = 12;
static const auto k_batch_unroll = 1;
static const auto k_max_samples = 1e9;
static const auto k_warmup_samples = 2;
static const auto k_batch_samples = 628;
static const auto k_samples_threshold = 12;
static const auto k_batch_threshold = 65536;
static const auto k_min_cpu_time = 1000 * 1e6;
static const auto k_warmup_threshold = 500000;
static const auto epoch = std::chrono::high_resolution_clock::now();

struct k_options {
  u64 min_samples = k_min_samples;
  u64 max_samples = k_max_samples;
  u64 batch_unroll = k_batch_unroll;
  f64 min_cpu_time = k_min_cpu_time;
  u64 batch_samples = k_batch_samples;
  u64 warmup_samples = k_warmup_samples;
  f64 batch_threshold = k_batch_threshold;
  f64 warmup_threshold = k_warmup_threshold;
  u64 samples_threshold = k_samples_threshold;
};

inline const f64 now() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
           std::chrono::high_resolution_clock::now() - epoch
  )
    .count();
}

// Update the run_with_timeout function

template <typename Func>
std::pair<bool, f64> run_with_timeout(Func&& fn, u64 timelimit_ns) {
  if (timelimit_ns == 0) {
    const auto t0 = now();
    fn();
    const auto t1 = now();
    return {false, t1 - t0};
  }

  std::atomic<f64> execution_time{0.0};
  std::atomic<bool> is_done{false};

  std::thread worker_thread([&]() {
    try {
      const auto t0 = now();
      fn();
      const auto t1 = now();
      execution_time.store(t1 - t0);
    } catch (...) {
      // Catch any exceptions
    }
    is_done.store(true);
  });

  // Fixed sleep interval to avoid consuming too many resources
  const auto check_interval = std::chrono::milliseconds(10);
  const auto max_wait = std::chrono::nanoseconds(timelimit_ns);
  std::chrono::nanoseconds elapsed(0);

  while (!is_done.load() && elapsed < max_wait && !timeout::requested()) {
    std::this_thread::sleep_for(check_interval);
    elapsed += check_interval;
  }

  bool timed_out = elapsed >= max_wait || timeout::requested();

  if (timed_out) {
    worker_thread.detach();  // Detach to avoid blocking
    return {true, 0.0};
  } else {
    if (worker_thread.joinable()) {
      worker_thread.join();
    }
    return {false, execution_time.load()};
  }
}

inline std::string repeat(int n, const std::string& input_string) {
  std::ostringstream os;
  for (int i = 0; i < n; i++)
    os << input_string;
  return os.str();
}

inline const k_stats
  fn(std::function<void()> fn, const k_options opts = k_options(), u64 timelimit_ns = 0) {
  k_stats stats;
  bool batch = false;

  // Start the group timer at the beginning of the benchmark
  timeout::reset();
  timeout::ScopedTimer timer(timelimit_ns);

warmup: {
  // Check for timeout before each operation
  if (timeout::requested()) {
    stats.timeout = true;
    return stats;
  }

  // Test if the function is fast or slow
  auto t0 = now();
  fn();
  auto t1 = now();
  auto duration = t1 - t0;

  if (duration <= opts.warmup_threshold) {
    for (auto o = 0; o < opts.warmup_samples; o++) {
      // Check for timeout before each operation
      if (timeout::requested()) {
        stats.timeout = true;
        return stats;
      }

      t0 = now();
      fn();
      t1 = now();
      duration = t1 - t0;

      if ((batch = duration <= opts.batch_threshold))
        break;
    }
  }
}
  if (!batch) {
    f64 t = 0.0;

    for (auto _ = 0; _ < opts.max_samples; _++) {
      if (_ >= opts.min_samples && t >= opts.min_cpu_time)
        break;

      // Check for timeout before each operation
      if (timeout::requested()) {
        stats.timeout = true;
        return stats;
      }

      auto t0 = now();
      fn();
      auto t1 = now();
      auto duration = t1 - t0;

      t += duration;
      stats.samples.push_back(duration);
    }
  } else {
    f64 t = 0.0;

    for (auto _ = 0; _ < opts.max_samples; _++) {
      if (_ >= opts.min_samples && t >= opts.min_cpu_time)
        break;

      // Check for timeout before each batch
      if (timeout::requested()) {
        stats.timeout = true;
        return stats;
      }

      auto t0 = now();
      for (auto o = 0; o < (opts.batch_samples / opts.batch_unroll); o++) {
        for (auto u = 0; u < opts.batch_unroll; u++)
          fn();
      }
      auto t1 = now();
      auto duration = t1 - t0;

      t += duration;
      stats.samples.push_back(duration / opts.batch_samples);
    }
  };

  // Stop the timer once we're done
  timeout::stop_timer();

  if (stats.samples.empty()) {
    if (!stats.timeout) {
      stats.min = stats.max = stats.avg = 0.0;
      stats.p25 = stats.p50 = stats.p75 = stats.p99 = stats.p999 = 0.0;
      stats.ticks = 0;
      stats.samples.push_back(0.0);
    }
    return stats;
  }

  std::sort(stats.samples.begin(), stats.samples.end());
  if (stats.samples.size() > opts.samples_threshold)
    stats.samples = std::vector<f64>(stats.samples.begin() + 2, stats.samples.end() - 2);

  stats.max = stats.samples.back();
  stats.min = stats.samples.front();
  stats.p25 = stats.samples[.25 * (stats.samples.size() - 1)];
  stats.p50 = stats.samples[.50 * (stats.samples.size() - 1)];
  stats.p75 = stats.samples[.75 * (stats.samples.size() - 1)];
  stats.p99 = stats.samples[.99 * (stats.samples.size() - 1)];
  stats.p999 = stats.samples[.999 * (stats.samples.size() - 1)];
  stats.ticks = stats.samples.size() * (!batch ? 1 : opts.batch_samples);
  stats.avg =
    std::accumulate(stats.samples.begin(), stats.samples.end(), 0.0) / stats.samples.size();

  return stats;
}

// Update the measurement helper to pass the timelimit
template <typename F>
inline const k_stats measure_function(F&& fn, u64 timelimit_ns = 0) {
  return lib::fn(fn, lib::k_options(), timelimit_ns);
}
}  // namespace lib
}  // namespace mitata